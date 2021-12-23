#include "VFS.hpp"
#include "../klog.h"
#include "MBR.h"
#include "../BlockDevice.hpp"
#include "../liballoc.h"
#include "../runtime.h"
#include "Ext2.hpp"
#include <cstring>

bool VFS::init(){
    kprintf("init VFS\n");
    return true;
}

bool VFS::testPartition(BlockDevice& dev, const PartitionTableEntry* ent){
  if(ent->active == 0 && ent->systemID == 0) {
    return false;
  }
  auto mountPointOrErr = Ext2FS::probe(dev, ent->lbaStart);
  if(mountPointOrErr){
    _mountables.push_back(mountPointOrErr.value);
  }
  return mountPointOrErr.error;
}

bool VFS::inpectDev(BlockDevice& dev){
  char buf[512] = {0};
  auto readRet = dev.read(0, buf, 512);
  if(readRet == 512){
      const MBR* mbr =(const MBR*) buf;
      // validBoot should be == 0XAA55 and diskID != 0
      if(mbr->diskID != 0 && mbr->validBoot == 0XAA55){
        testPartition(dev, &mbr->part1);
        testPartition(dev, &mbr->part2);
        testPartition(dev, &mbr->part3);
        testPartition(dev, &mbr->part4);
      }else {
        // single partition disk
        kprintf("No MBR, mount disk directly\n");
      }
  }
  return false;
}

bool VFS::mount(const VFS::FileSystem*fs, const char* path){
  kprintf("Mount device at path '%s'\n", path);
  _rootFS = fs;
  return false;
}

bool VFS::testRead(){
  return enumInodeDir(2, [this](const char* name, uint32_t inodeID){
    if(inodeID == 2){
      return; 
    }
    if(strcmp(name, ".") == 0){
      return;
    }
    if(strcmp(name, "..") == 0){
      return;
    }
    kprintf("Entry '%s' inode id = %u\n", name, inodeID);
    enumInodeDir(inodeID, [](const char* name, uint32_t inodeID){
      if(strcmp(name, ".") == 0){
        return;
      }
      if(strcmp(name, "..") == 0){
        return;
      }
      kprintf("\tEntry '%s' inode id = %u\n", name, inodeID);
    });
  });
}

bool VFS::enumInodeDir(uint32_t inodeID, std::function<void(const char*, uint32_t inodeID)> entryCallback){
  inode_t* ino = (inode_t*) kmalloc(sizeof(inode_t)); 
  if(!ino){
    return false;
  }
  if(!_rootFS->read(ino, inodeID)){
    kfree(ino);
    return false;
  }
	if(!ino->isDir()){
		kfree(ino);
    return false;
	}
  char tmpName[256] = "";
  for(int i = 0;i < 12; i++){
		uint32_t blockID = ino->dbp[i];
		if(blockID == 0){
      break;
    }
    uint8_t *buf = (uint8_t*) kmalloc(4096);
    _rootFS->readBlock(buf, 4096, blockID);
    ext2_dir* dir = (ext2_dir*) buf;
    while(dir->inode != 0) {
      if(dir->namelength < 255){
        memcpy(tmpName, &dir->reserved+1, dir->namelength);
        tmpName[dir->namelength] = 0;
        entryCallback(tmpName, dir->inode);
      }
      dir = (ext2_dir *)((uint64_t)dir + dir->size);
      ptrdiff_t dif = (char*) dir - (char*) buf;
      if( dif >= _rootFS->priv.blocksize){
        kfree(buf);
        break;
      }
    }
    kfree(buf);
  }
  kfree(ino);
  return true;
}

bool VFS::readFile(uint32_t inodeID, std::function<bool(size_t, size_t, size_t,const uint8_t*)> onData){
  kprintf("Test read file at inode %zu\n", inodeID);

  inode_t* inode = (inode_t*) kmalloc(sizeof(inode_t)); 
  if(!inode){
    return false;
  }
  if(!_rootFS->read(inode, inodeID)){
    kfree(inode);
    return false;
  }
	if(!inode->isFile()){
		kfree(inode);
    return false;
	}
  uint8_t *tempBuf = (uint8_t*) kmalloc(4096);
  if(!tempBuf){
      kprintf("unable to kmalloc tempBuf\n");
      kfree(inode);
      return true; 
  }
  kprintf("inode is a file, size=%zi\n", inode->size);
  size_t readPos = 0;
  bool done = false;
  bool ret = false;
  const size_t numBytes = 4096;
  uint32_t *block = nullptr;
  while(!done){
    size_t indexOfBlockToRead = (size_t)readPos / _rootFS->priv.blocksize;
    if(indexOfBlockToRead<12){
      uint32_t b = inode->dbp[indexOfBlockToRead];
      if(b==0){
          kprintf("EOF For read\n");
          ret = true;
          break;
      }
      if(b > _rootFS->priv.sb.blocks) {
        kprintf("block %d outside range (max: %d)!\n",  b, _rootFS->priv.sb.blocks);
      }
      size_t sizeToCopy = inode->size - readPos;
      if(sizeToCopy > numBytes){
          sizeToCopy = numBytes;
      }
      if(!_rootFS->readBlock(tempBuf, numBytes, b)){
          ret = false;
          break;
      }
      if(!onData(readPos,sizeToCopy, inode->size, tempBuf)){
        ret = true;
        break;
      }
      readPos += sizeToCopy;
      if(readPos >= inode->size){
        ret = true;
        break;
      }
    }else{ 
      // indexOfBlockToRead> >= 12
      if(inode->doubly_block)
      {
          kprintf("Doubly block, to implement :)\n");
          assert(0);
      }
      if(inode->triply_block)
      {
          kprintf("Triply block, to implement :)\n");
          assert(0);
      }
      if(inode->singly_block)
      {
        auto blockIndex = indexOfBlockToRead - 12;
        if(!block){
          block = (uint32_t *) kmalloc(4096);
        }
        if(!block){
          ret = false;
          break;
        }
        if(!_rootFS->readBlock((uint8_t*) block, 4096, inode->singly_block)){
          kprintf("Read block error\n");
          ret = false;
          break;
        }
        if(block[blockIndex] == 0){
            kprintf("block[%i] is 0 ?\n", blockIndex);
            continue;
        }else{
          auto blockId = block[blockIndex];
          _rootFS->readBlock((uint8_t *)block, 4096, blockId);
          size_t sizeToCopy = inode->size - readPos;
          if(sizeToCopy > numBytes){
              sizeToCopy = numBytes;
          }
          if(!onData(readPos,sizeToCopy, inode->size, (uint8_t *)block)){
            ret = true;
            break;
          }
          readPos += sizeToCopy;
          if(readPos >= inode->size){
            ret = true;
            break;
          }
        }
      }
    }
  }// end while(!done)


  kfree(tempBuf);
	kfree(inode);
  if(block){
    kfree(block);
  }
  return ret;
}