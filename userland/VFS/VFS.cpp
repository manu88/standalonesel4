#include "VFS.hpp"
#include "../BlockDevice.hpp"
#include "../klog.h"
#include "../liballoc.h"
#include "../runtime.h"
#include "Ext2.hpp"
#include "MBR.h"
#include <cstring>

bool VFS::init() {
  kprintf("init VFS\n");
  return true;
}

bool VFS::testPartition(BlockDevice &dev, const PartitionTableEntry *ent) {
  if (ent->active == 0 && ent->systemID == 0) {
    return false;
  }
  auto mountPointOrErr = Ext2FS::probe(dev, ent->lbaStart);
  if (mountPointOrErr) {
    _mountables.push_back(mountPointOrErr.value);
  }
  return mountPointOrErr.error;
}

bool VFS::inpectDev(BlockDevice &dev) {
  uint8_t buf[512] = {0};
  auto readRet = dev.read(0, buf, 512);
  if (readRet == 512) {
    const MBR *mbr = (const MBR *)buf;
    // validBoot should be == 0XAA55 and diskID != 0
    if (mbr->diskID != 0 && mbr->validBoot == 0XAA55) {
      testPartition(dev, &mbr->part1);
      testPartition(dev, &mbr->part2);
      testPartition(dev, &mbr->part3);
      testPartition(dev, &mbr->part4);
    } else {
      // single partition disk
      kprintf("No MBR, mount disk directly\n");
    }
  }
  return false;
}

bool VFS::mount(const VFS::FileSystem *fs, const char *path) {
  kprintf("Mount device at path '%s'\n", path);
  _rootFS = fs;
  return false;
}

Optional<VFS::File> VFS::open(uint32_t inodeID) {
  inode_t *inode = (inode_t *)kmalloc(sizeof(inode_t));
  if (!inode) {
    return Optional<VFS::File>();
  }
  if (!_rootFS->read(inode, inodeID)) {
    kfree(inode);
    return Optional<VFS::File>();
  }
  File f;
  f.inode = inode;
  kprintf("VFS::Open inode\n");
  kprintf("type: 0X%X\n", inode->type);
  kprintf("uid: 0X%X\n", inode->uid);
  kprintf("size: 0X%X\n", inode->size);
  kprintf("last_access: 0X%X\n", inode->last_access);
  kprintf("create_time: 0X%X\n", inode->create_time);
  kprintf("last_modif: 0X%X\n", inode->last_modif);
  kprintf("delete_time: 0X%X\n", inode->delete_time);
  kprintf("gid: 0X%X\n", inode->gid);
  kprintf("hardlinks: 0X%X\n", inode->hardlinks);
  kprintf("disk_sectors: 0X%X\n", inode->disk_sectors);
  kprintf("flags: 0X%X\n", inode->flags);
  kprintf("ossv1: 0X%X\n", inode->ossv1);
  for(int i=0;i<12;i++){
    kprintf("dbp[%i]: 0X%X\n",i, inode->dbp[i]);
  }
  kprintf("singly_block: 0X%X\n", inode->singly_block);
  kprintf("doubly_block: 0X%X\n", inode->doubly_block);
  kprintf("triply_block: 0X%X\n", inode->triply_block);
  kprintf("gen_no: 0X%X\n", inode->gen_no);
  kprintf("reserved1: 0X%X\n", inode->reserved1);
  kprintf("reserved2: 0X%X\n", inode->reserved2);
  kprintf("fragment_block: 0X%X\n", inode->fragment_block);

  return Optional<VFS::File>(f);
}

bool VFS::close(VFS::File &f) {
  if (f.inode) {
    kfree(f.inode);
  }
  return false;
}
ssize_t VFS::read(VFS::File &f, uint8_t *buf, size_t bufSize) {
  assert(buf);
  if (f.inode == nullptr) {
    return -1;
  }
  if (f.pos >= f.inode->size) {
    return 0;
  }
  size_t indexOfBlockToRead = (size_t)f.pos / _rootFS->priv.blocksize;
  if (indexOfBlockToRead < 12) {
    uint32_t blockID = f.inode->dbp[indexOfBlockToRead];
    if (blockID == 0) {
      return 0; // EOF
    }
    if (blockID > _rootFS->priv.sb.blocks) {
      kprintf("block %d outside range (max: %d)!\n", blockID,
              _rootFS->priv.sb.blocks);
    }
    uint8_t *buffer = buf;
    size_t bufferSize = bufSize;
    if (bufSize < 4096) {
      buffer = (uint8_t *)kmalloc(4096);
      assert(buffer);
      bufferSize = 4096;
    }
    assert(bufferSize >= 4096);
    if (!_rootFS->readBlock(buffer, bufferSize, blockID)) {
      if (buffer != buf) {
        kfree(buffer);
      }
      return -1;
    }
    size_t sizeToCopy = f.inode->size - f.pos;
    if (sizeToCopy > bufSize) {
      sizeToCopy = bufSize;
    }
    size_t chunkToCopy = f.pos % _rootFS->priv.blocksize;
    memcpy(buf, buffer + chunkToCopy, sizeToCopy);
    f.pos += sizeToCopy;
    if (buffer != buf) {
      kfree(buffer);
    }
    return sizeToCopy;
  }else{
    assert(0);
  }
  return -1;
}

bool VFS::testRead() {
  return enumInodeDir(2, [this](const char *name, uint32_t inodeID) {
    if (inodeID == 2) {
      return;
    }
    if (strcmp(name, ".") == 0) {
      return;
    }
    if (strcmp(name, "..") == 0) {
      return;
    }
    kprintf("Entry '%s' inode id = %u\n", name, inodeID);
    enumInodeDir(inodeID, [](const char *name, uint32_t inodeID) {
      if (strcmp(name, ".") == 0) {
        return;
      }
      if (strcmp(name, "..") == 0) {
        return;
      }
      kprintf("\tEntry '%s' inode id = %u\n", name, inodeID);
    });
  });
}

bool VFS::enumInodeDir(
    uint32_t inodeID,
    std::function<void(const char *, uint32_t inodeID)> entryCallback) {
  inode_t *ino = (inode_t *)kmalloc(sizeof(inode_t));
  if (!ino) {
    return false;
  }
  if (!_rootFS->read(ino, inodeID)) {
    kfree(ino);
    return false;
  }
  if (!ino->isDir()) {
    kfree(ino);
    return false;
  }
  char tmpName[256] = "";
  for (int i = 0; i < 12; i++) {
    uint32_t blockID = ino->dbp[i];
    if (blockID == 0) {
      break;
    }
    uint8_t *buf = (uint8_t *)kmalloc(4096);
    _rootFS->readBlock(buf, 4096, blockID);
    ext2_dir *dir = (ext2_dir *)buf;
    while (dir->inode != 0) {
      if (dir->namelength < 255) {
        memcpy(tmpName, &dir->reserved + 1, dir->namelength);
        tmpName[dir->namelength] = 0;
        entryCallback(tmpName, dir->inode);
      }
      dir = (ext2_dir *)((uint64_t)dir + dir->size);
      ptrdiff_t dif = (char *)dir - (char *)buf;
      if (dif >= _rootFS->priv.blocksize) {
        kfree(buf);
        break;
      }
    }
    kfree(buf);
  }
  kfree(ino);
  return true;
}
