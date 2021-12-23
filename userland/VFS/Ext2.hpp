#pragma once
#include "../lib/expected.hpp"
#include "VFS.hpp"
#include "ext2_defs.h"
#include <cstddef>

struct BlockDevice;

class Ext2FS {
public:
  struct Operations : public VFS::Operations {
    bool read(const VFS::FileSystem &fs, inode_t *inode_buf,
              uint32_t inode) final;
    bool readBlock(const VFS::FileSystem &, uint8_t *buf, size_t bufSize,
                   uint32_t blockID) final;
  };
  using OptionalFileSystem = Expected<VFS::FileSystem, bool>;
  static OptionalFileSystem probe(BlockDevice &dev, size_t lbaStart);

  static Operations ops;

private:
  static bool testRead(VFS::FileSystem &);
};