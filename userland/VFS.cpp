#include "VFS.hpp"
#include "klog.h"
#include "MBR.h"
#include "BlockDevice.hpp"
#include "liballoc.h"
#include "ext2.h"

bool VFS::init(){
    kprintf("init VFS\n");
    return true;
}

uint8_t Ext2Probe(BlockDevice& dev, size_t lbaStart){
	uint8_t *buf = (uint8_t *)kmalloc(1024);
  ssize_t ret = dev.read(lbaStart + 2, (char*) buf, 512);
  ret = dev.read(lbaStart + 3, (char*) buf+512, 512);

	superblock_t *sb = (superblock_t *)buf;
	if(sb->ext2_sig != EXT2_SIGNATURE)
	{
		kprintf("Invalid EXT2 signature, have: 0x%x!\n", sb->ext2_sig);
		kfree(buf);
		return 0;
	}
	kprintf("Valid EXT2 signature!\n");
#if 0
  ext2_priv_data *priv = getExtPriv();
	priv->lbaStart = lbaStart;

	memcpy(&priv->sb, sb, sizeof(superblock_t));
	/* Calculate volume length */
	uint32_t blocksize = 1024 << sb->blocksize_hint;
	kprintf("Size of a block: %d bytes\n", blocksize);
	priv->blocksize = blocksize;
	priv->inodes_per_block = blocksize / sizeof(inode_t);
	priv->sectors_per_block = blocksize / 512;
	kprintf("Size of volume: %d bytes\n", blocksize*(sb->blocks));
	/* Calculate the number of block groups */
	uint32_t number_of_bgs0 = sb->blocks / sb->blocks_in_blockgroup;
	if(!number_of_bgs0) number_of_bgs0 = 1;
	kprintf("There are %d block group(s).\n", number_of_bgs0);
	priv->number_of_bgs = number_of_bgs0;
	/* Now, we have the size of a block,
	 * calculate the location of the Block Group Descriptor
	 * The BGDT is located directly after the SB, so obtain the
	 * block of the SB first. This is located in the SB.
	 */
	uint32_t block_bgdt = sb->superblock_id + (sizeof(superblock_t) / blocksize);
	priv->first_bgd = 1;//block_bgdt;
  kprintf("first_bgd is at %u\n", priv->first_bgd);
	kprintf("Device %s is with EXT2 filesystem. Probe successful.\n", dev->name);
#endif
	kfree(buf);

	return 1;
}

static int testPartition(BlockDevice& dev, const PartitionTableEntry* ent)
{
  if(ent->active == 0 && ent->systemID == 0) {
    kprintf("\t-->Empty partition table\n");
    return -1;
  } else {
    kprintf("\t-->active %X systemID %X num sectors %d lba start %d start sector %X\n", ent->active, ent->systemID, ent->numSectors, ent->lbaStart, ent->startSector);
    if(Ext2Probe(dev, ent->lbaStart)){
#if 0
      if(Ext2Mount(dev)) {
        kprintf("[VFSMount] ext2\n");
          getExt2FS()->data = dev;
          int err = 0;
          VFSMount(getExt2FS(), "/ext", &err);
          return err;
      }
      kprintf("ext2_mount error \n");
#endif
    }else{
      kprintf("ext2_probe error \n");
      return -1;
    }
  }

}

bool VFS::inpectDev(BlockDevice& dev){
  char buf[512] = {0};
  auto readRet = dev.read(0, buf, 512);
  kprintf("Did read %zi bytes\n", readRet);
  if(readRet == 512){
      const MBR* mbr =(const MBR*) buf;
      kprintf("MBR diskID=%X validboot=%X\n", mbr->diskID, mbr->validBoot);
      // validBoot should be == 0XAA55 and diskID != 0
      if(mbr->diskID != 0 && mbr->validBoot == 0XAA55)
      {
          kprintf("Found a valid MBR, check partitions\n");
          kprintf("Check partition1\n");
          testPartition(dev, &mbr->part1);
          kprintf("Check partition2\n");
          testPartition(dev, &mbr->part2);
          kprintf("Check partition3\n");
          testPartition(dev, &mbr->part3);
          kprintf("Check partition4\n");
          testPartition(dev, &mbr->part4);
      }
      else // single partition disk
      {
          kprintf("No MBR, mount disk directly\n");
      }
  }
}