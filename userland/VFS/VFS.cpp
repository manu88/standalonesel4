#include "VFS.hpp"
#include "../klog.h"
#include "MBR.h"
#include "../BlockDevice.hpp"
#include "../liballoc.h"
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
    kprintf("Entry '%s' inode id = %u\n", name, inodeID);
    if(inodeID == 2){
      return; 
    }
    if(strcmp(name, ".") == 0){
      return;
    }
    if(strcmp(name, "..") == 0){
      return;
    }
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