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
    kprintf("\t-->Empty partition table\n");
    return false;
  }
  kprintf("\t-->active %X systemID %X num sectors %d lba start %d start sector %X\n", ent->active, ent->systemID, ent->numSectors, ent->lbaStart, ent->startSector);
  auto mountPointOrErr = Ext2FS::probe(dev, ent->lbaStart);
  if(mountPointOrErr){
    _mountables.push_back(mountPointOrErr.value);
  }
  return mountPointOrErr.error;
}

bool VFS::inpectDev(BlockDevice& dev){
  char buf[512] = {0};
  auto readRet = dev.read(0, buf, 512);
  kprintf("Did read %zi bytes\n", readRet);
  if(readRet == 512){
      const MBR* mbr =(const MBR*) buf;
      kprintf("MBR diskID=%X validboot=%X\n", mbr->diskID, mbr->validBoot);
      // validBoot should be == 0XAA55 and diskID != 0
      if(mbr->diskID != 0 && mbr->validBoot == 0XAA55){
        kprintf("Found a valid MBR, check partitions\n");
        kprintf("Check partition1\n");
        testPartition(dev, &mbr->part1);

        kprintf("Check partition2\n");
        testPartition(dev, &mbr->part2);

        kprintf("Check partition3\n");
        testPartition(dev, &mbr->part3);

        kprintf("Check partition4\n");
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
  inode_t ino;
  if(!_rootFS->read(&ino, 2)){
    kprintf("VFS::testRead error getting superblock\n");
  }
  kprintf("VFS::testRead OK\n");

	if(!ino.isDir()){
		kprintf("FATAL: Root directory is not a directory!\n");
		return false;
	}
  kprintf("Root directory IS a directory!\n");

  char tmpName[256] = "";
  for(int i = 0;i < 12; i++){
		uint32_t blockID = ino.dbp[i];
		if(blockID == 0){
      break;
    }
    uint8_t *buf = (uint8_t*) kmalloc(512);
    _rootFS->readBlock(buf, blockID);
    ext2_dir* dir = (ext2_dir*) buf;
    while(dir->inode != 0) {
      if(dir->namelength < 255){
        memcpy(tmpName, &dir->reserved+1, dir->namelength);
        tmpName[dir->namelength] = 0;
        kprintf("Got file '%s'\n", tmpName);
      }
      dir = (ext2_dir *)((uint64_t)dir + dir->size);
      ptrdiff_t dif = (char*) dir - (char*) buf;
      if( dif >= _rootFS->priv.blocksize){
        break;
      }
    }
    kfree(buf);
  }

  return true;
}