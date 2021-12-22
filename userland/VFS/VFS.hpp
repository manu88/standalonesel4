#pragma once
#include "ext2_defs.h"
#include "../lib/vector.hpp"
#include <sys/types.h>
#include <functional>

#define INODE_TYPE_FIFO 0x1000
#define INODE_TYPE_CHAR_DEV 0x2000
#define INODE_TYPE_DIRECTORY 0x4000
#define INODE_TYPE_BLOCK_DEV 0x6000
#define INODE_TYPE_FILE 0x8000
#define INODE_TYPE_SYMLINK 0xA000
#define INODE_TYPE_SOCKET 0xC000

typedef struct 
{
	uint16_t type;
	uint16_t uid;
	uint32_t size;
	uint32_t last_access;
	uint32_t create_time;
	uint32_t last_modif;
	uint32_t delete_time;
	uint16_t gid;
	uint16_t hardlinks;
	uint32_t disk_sectors;
	uint32_t flags;
	uint32_t ossv1;
	uint32_t dbp[12];
	uint32_t singly_block;
	uint32_t doubly_block;
	uint32_t triply_block;
	uint32_t gen_no;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t fragment_block;
	uint8_t ossv2[12];

    bool isDir()const noexcept{
        return (type & 0xF000) == INODE_TYPE_DIRECTORY;
    }

} __attribute__((packed)) inode_t;

struct BlockDevice;
typedef struct _PartitionTableEntry PartitionTableEntry;


class VFS{
public:
    struct FileSystem;
    struct Operations{
        virtual ~Operations(){}
        virtual bool read(const FileSystem &, inode_t *, uint32_t) {return false;}
        virtual bool readBlock(const FileSystem &, uint8_t *, size_t , uint32_t) {return false;}
    };

	struct FileSystem{
        bool read(inode_t *ino, uint32_t inodeID) const {return ops->read(*this, ino, inodeID);}
        bool readBlock(uint8_t *buf, size_t bufSize, uint32_t blockID) const {return ops->readBlock(*this, buf, bufSize, blockID);}
		FileSystem(int = 0){}
		ext2_priv_data priv;
		BlockDevice* dev = nullptr;

        Operations* ops = nullptr;
	};

    bool init();
    bool inpectDev(BlockDevice&);

    bool mount(const FileSystem*, const char* path);

    const vector<FileSystem>& getMountables() const noexcept{
        return _mountables;
    }

    bool testRead();
    bool enumInodeDir(uint32_t inodeID, std::function<void(const char*, uint32_t inodeID)> entryCallback);

private:
    bool testPartition(BlockDevice& dev, const PartitionTableEntry* ent);
    vector<FileSystem> _mountables;

    const FileSystem* _rootFS = nullptr;

};