#include "Ext2.hpp"
#include "../BlockDevice.hpp"
#include "../liballoc.h"
#include "../klog.h"
#include <cstring>
#include "../runtime.h"

static bool doReadBlock(uint8_t *buf, size_t bufSize, uint32_t block, BlockDevice& dev, const ext2_priv_data *priv){
	uint32_t sectors_per_block = priv->sectors_per_block;
	if(!sectors_per_block){
      sectors_per_block = 1;
  }
  uint32_t startSect = block*sectors_per_block;
  uint32_t numSectors = block*sectors_per_block + sectors_per_block - startSect;
  assert((numSectors*512) <= bufSize);
  buf[(numSectors*512)-1] = 0;
  uint8_t *bufPos = buf;
  size_t acc = 0;

  for(uint32_t i=0;i<numSectors;i++){
    ssize_t ret = dev.read(priv->lbaStart+startSect+i, (char*)bufPos, 512);
    if(ret <= 0){
      kprintf("doReadBlock: read error for sector %i\n", i);
      return false;
    }
    bufPos += ret;
    acc += ret;
  }
	return true;
}

Ext2FS::Operations Ext2FS::ops;

bool Ext2FS::Operations::read(const VFS::FileSystem &fs, inode_t *inode_buf, uint32_t inode){
	uint32_t bg = (inode - 1) / fs.priv.sb.inodes_in_blockgroup;
	uint32_t i = 0;

  size_t blockBufSize = 4096;
  uint8_t* blockBuf = (uint8_t*) kmalloc(blockBufSize);
  if(blockBuf == NULL){
      return false;
  }

  if(!readBlock(fs, blockBuf, blockBufSize, fs.priv.first_bgd)){
    kfree(blockBuf);
    return false;
  }
	block_group_desc_t *bgd = (block_group_desc_t*)blockBuf;

	for(i = 0; i < bg; i++){
		bgd++;
  }

	uint32_t index = (inode - 1) % fs.priv.sb.inodes_in_blockgroup;
	uint32_t block = (index * sizeof(inode_t))/ fs.priv.blocksize;

  if(!readBlock(fs, blockBuf, blockBufSize, bgd->block_of_inode_table + block)){
    kfree(blockBuf);
    return false;
  }

	inode_t* _inode = (inode_t *)blockBuf;
	index = index % fs.priv.inodes_per_block;

	for(i = 0; i < index; i++){
		_inode++;
	}

	memcpy(inode_buf, _inode, sizeof(inode_t));
  kfree(blockBuf);
	return true;
}


bool Ext2FS::Operations::readBlock(const VFS::FileSystem &vfs, uint8_t *buf, size_t bufSize, uint32_t blockID){
	bool r = doReadBlock(buf, bufSize, blockID, *vfs.dev, &vfs.priv); 
  if(!r){
		return doReadBlock(buf, bufSize, blockID, *vfs.dev, &vfs.priv);
	}
	return r;
}

Ext2FS::OptionalFileSystem Ext2FS::probe(BlockDevice& dev, size_t lbaStart){
  uint8_t *buf = (uint8_t *)kmalloc(1024);
  ssize_t ret = dev.read(lbaStart + 2, (char*) buf, 512);
  if(ret!= 512){
    kfree(buf);
    return unexpected<VFS::FileSystem, bool>(false);
  }
  ret = dev.read(lbaStart + 3, (char*) buf+512, 512);
  if(ret!= 512){
    kfree(buf);
    return unexpected<VFS::FileSystem, bool>(false);
  }
	superblock_t *sb = (superblock_t *)buf;
	if(sb->ext2_sig != EXT2_SIGNATURE){
		kprintf("Invalid EXT2 signature, have: 0x%x!\n", sb->ext2_sig);
		kfree(buf);
		return unexpected<VFS::FileSystem, bool>(false);
	}
	kprintf("Valid EXT2 signature!\n");
  ext2_priv_data priv;
	priv.lbaStart = lbaStart;

	memcpy(&priv.sb, sb, sizeof(superblock_t));
	/* Calculate volume length */
	uint32_t blocksize = 1024 << sb->blocksize_hint;
	kprintf("Size of a block: %d bytes\n", blocksize);
	priv.blocksize = blocksize;
	priv.inodes_per_block = blocksize / sizeof(inode_t);
	priv.sectors_per_block = blocksize / 512;
	kprintf("Size of volume: %d Mb\n", (blocksize*(sb->blocks))/(1024*1024));
	/* Calculate the number of block groups */
	uint32_t number_of_bgs0 = sb->blocks / sb->blocks_in_blockgroup;
	if(!number_of_bgs0) {
    number_of_bgs0 = 1;
  }
	kprintf("There are %d block group(s).\n", number_of_bgs0);
	priv.number_of_bgs = number_of_bgs0;
	/* Now, we have the size of a block,
	 * calculate the location of the Block Group Descriptor
	 * The BGDT is located directly after the SB, so obtain the
	 * block of the SB first. This is located in the SB.
	 */
	//uint32_t block_bgdt = sb->superblock_id + (sizeof(superblock_t) / blocksize);
	priv.first_bgd = 1;//block_bgdt;
  kprintf("first_bgd is at %u\n", priv.first_bgd);
	kprintf("Device has EXT2 filesystem. Probe successful.\n");
	kfree(buf);

  VFS::FileSystem fs;
  fs.priv = priv;
  fs.dev = &dev;
  fs.ops = &Ext2FS::ops;
	return success<VFS::FileSystem, bool>(fs, true);
}
