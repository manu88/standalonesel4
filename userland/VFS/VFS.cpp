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