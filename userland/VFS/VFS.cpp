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

ext2_priv_data _priv;

static bool doReadBlock(uint8_t *buf, uint32_t block, BlockDevice& dev, ext2_priv_data *priv){
	uint32_t sectors_per_block = priv->sectors_per_block;
	if(!sectors_per_block){
      sectors_per_block = 1;
  }
  uint32_t startSect = block*sectors_per_block;
  uint32_t numSectors = block*sectors_per_block + sectors_per_block - startSect;

  buf[(numSectors*512)-1] = 0;
  uint8_t *bufPos = buf;
  size_t acc = 0;

  for(uint32_t i=0;i<numSectors;i++){
    ssize_t ret = dev.read(priv->lbaStart+startSect+i, (char*)bufPos, 512);

    if(ret <= 0){
      return false;
    }
    bufPos += ret;
    acc += ret;
  }
	return true;
}

bool Ext2ReadBlock(BlockDevice& dev, uint8_t *buf, uint32_t blockID)
{
  kprintf("Reading block id %zi\n", blockID);
	bool r = doReadBlock(buf, blockID, dev, &_priv); 
  if(!r)
	{
		kprintf("Ext2ReadBlock read error, retry once\n");
		return doReadBlock(buf, blockID, dev, &_priv);
	}
	return r;
}

bool Ext2ReadInode(BlockDevice& dev, inode_t *inode_buf, uint32_t inode)
{
  ext2_priv_data *priv = &_priv;
	uint32_t bg = (inode - 1) / priv->sb.inodes_in_blockgroup;
	uint32_t i = 0;

  uint8_t* block_buf = (uint8_t*) kmalloc(512);
  if(block_buf == NULL)
  {
      return false;
  }
  if(!Ext2ReadBlock(dev, block_buf,  priv->first_bgd)){
    kfree(block_buf);
    return false;
  }
	block_group_desc_t *bgd = (block_group_desc_t*)block_buf;

	for(i = 0; i < bg; i++){
		bgd++;
  }

	uint32_t index = (inode - 1) % priv->sb.inodes_in_blockgroup;

	uint32_t block = (index * sizeof(inode_t))/ priv->blocksize;

  if(!Ext2ReadBlock(dev, block_buf, bgd->block_of_inode_table + block)){
    kfree(block_buf);
    return false;
  }

	inode_t* _inode = (inode_t *)block_buf;
	index = index % priv->inodes_per_block;

	for(i = 0; i < index; i++){
		_inode++;
	}

	memcpy(inode_buf, _inode, sizeof(inode_t));
	return true;
}

bool Ext2Mount(BlockDevice& dev){
	kprintf("Mounting ext2 on device\n");
	inode_t ino;
	if(Ext2ReadInode(dev, &ino, 2) == 0){
		return false;
	}
	if((ino.type & 0xF000) != INODE_TYPE_DIRECTORY){
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
    uint8_t buf[4096] = {0};// = (uint8_t*) kmalloc(1024);
    Ext2ReadBlock(dev, buf, blockID);
    ext2_dir* dir = (ext2_dir*) buf;
    while(dir->inode != 0) {
      if(dir->namelength < 255){
        memcpy(tmpName, &dir->reserved+1, dir->namelength);
        tmpName[dir->namelength] = 0;
        kprintf("Got file '%s'\n", tmpName);
      }
      dir = (ext2_dir *)((uint64_t)dir + dir->size);
      ptrdiff_t dif = (char*) dir - (char*) buf;
      if( dif >= _priv.blocksize){
        break;
      }
    }
  }
	return true;
}

bool Ext2Probe(BlockDevice& dev, size_t lbaStart){
	uint8_t *buf = (uint8_t *)kmalloc(1024);
  ssize_t ret = dev.read(lbaStart + 2, (char*) buf, 512);
  if(ret!= 512){
    kfree(buf);
    return false;
  }
  ret = dev.read(lbaStart + 3, (char*) buf+512, 512);
  if(ret!= 512){
    kfree(buf);
    return false;
  }
	superblock_t *sb = (superblock_t *)buf;
	if(sb->ext2_sig != EXT2_SIGNATURE){
		kprintf("Invalid EXT2 signature, have: 0x%x!\n", sb->ext2_sig);
		kfree(buf);
		return false;
	}
	kprintf("Valid EXT2 signature!\n");
  ext2_priv_data *priv = &_priv;
	priv->lbaStart = lbaStart;

	memcpy(&priv->sb, sb, sizeof(superblock_t));
	/* Calculate volume length */
	uint32_t blocksize = 1024 << sb->blocksize_hint;
	kprintf("Size of a block: %d bytes\n", blocksize);
	priv->blocksize = blocksize;
	priv->inodes_per_block = blocksize / sizeof(inode_t);
	priv->sectors_per_block = blocksize / 512;
	kprintf("Size of volume: %d Mb\n", (blocksize*(sb->blocks))/(1024*1024));
	/* Calculate the number of block groups */
	uint32_t number_of_bgs0 = sb->blocks / sb->blocks_in_blockgroup;
	if(!number_of_bgs0) {
    number_of_bgs0 = 1;
  }
	kprintf("There are %d block group(s).\n", number_of_bgs0);
	priv->number_of_bgs = number_of_bgs0;
	/* Now, we have the size of a block,
	 * calculate the location of the Block Group Descriptor
	 * The BGDT is located directly after the SB, so obtain the
	 * block of the SB first. This is located in the SB.
	 */
	//uint32_t block_bgdt = sb->superblock_id + (sizeof(superblock_t) / blocksize);
	priv->first_bgd = 1;//block_bgdt;
  kprintf("first_bgd is at %u\n", priv->first_bgd);
	kprintf("Device has EXT2 filesystem. Probe successful.\n");
	kfree(buf);

	return true;
}

static bool testPartition(BlockDevice& dev, const PartitionTableEntry* ent){
  if(ent->active == 0 && ent->systemID == 0) {
    kprintf("\t-->Empty partition table\n");
    return false;
  }

  kprintf("\t-->active %X systemID %X num sectors %d lba start %d start sector %X\n", ent->active, ent->systemID, ent->numSectors, ent->lbaStart, ent->startSector);
  if(Ext2Probe(dev, ent->lbaStart)){
    if(Ext2Mount(dev)) {
      kprintf("[VFSMount] ext2\n");
        return true;
    }
    kprintf("ext2_mount error \n");
    return false;
  }

  kprintf("ext2_probe error \n");
  return false;
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