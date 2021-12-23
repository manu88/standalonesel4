#pragma once
#include <cstddef>
#include <stdint.h>

#define EXT2_SIGNATURE 0xEF53

typedef struct {
  uint32_t inodes;
  uint32_t blocks;
  uint32_t reserved_for_root;
  uint32_t unallocatedblocks;
  uint32_t unallocatedinodes;
  uint32_t superblock_id;
  uint32_t blocksize_hint;    // shift by 1024 to the left
  uint32_t fragmentsize_hint; // shift by 1024 to left
  uint32_t blocks_in_blockgroup;
  uint32_t frags_in_blockgroup;
  uint32_t inodes_in_blockgroup;
  uint32_t last_mount;
  uint32_t last_write;
  uint16_t mounts_since_last_check;
  uint16_t max_mounts_since_last_check;
  uint16_t ext2_sig; // 0xEF53
  uint16_t state;
  uint16_t op_on_err;
  uint16_t minor_version;
  uint32_t last_check;
  uint32_t max_time_in_checks;
  uint32_t os_id;
  uint32_t major_version;
  uint16_t uuid;
  uint16_t gid;
  uint8_t unused[940];
} __attribute__((packed)) superblock_t;

typedef struct __ext2_priv_data {
  superblock_t sb;
  uint32_t first_bgd;
  uint32_t number_of_bgs;
  uint32_t blocksize;
  uint32_t sectors_per_block;
  uint32_t inodes_per_block;
  size_t lbaStart; // if in a partition, 0 otherwise

} __attribute__((packed)) ext2_priv_data;

typedef struct __ext2_dir_entry {
  uint32_t inode;
  uint16_t size;
  uint8_t namelength;
  uint8_t reserved;
  /* name here */
} __attribute__((packed)) ext2_dir;

typedef struct {
  uint32_t block_of_block_usage_bitmap;
  uint32_t block_of_inode_usage_bitmap;
  uint32_t block_of_inode_table;
  uint16_t num_of_unalloc_block;
  uint16_t num_of_unalloc_inode;
  uint16_t num_of_dirs;
  uint8_t unused[14];
} __attribute__((packed)) block_group_desc_t;