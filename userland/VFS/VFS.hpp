#pragma once
#include "Ext2.hpp"
#include "../lib/vector.hpp"

struct BlockDevice;
typedef struct _PartitionTableEntry PartitionTableEntry;

class VFS{
public:
    bool init();
    bool inpectDev(BlockDevice&);

    const vector<Ext2FS::Mountable>& getMountables() const noexcept{
        return _mountables;
    }

private:
    bool testPartition(BlockDevice& dev, const PartitionTableEntry* ent);
    vector<Ext2FS::Mountable> _mountables;
};