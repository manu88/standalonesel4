#pragma once
#include <stddef.h>
#include <cstdio>

typedef size_t seL4_Word;
typedef size_t seL4_CPtr;
typedef int seL4_Error;
typedef int sync_mutex_t;
#define seL4_NoError 0
#define seL4_InvalidArgument 1
#define seL4_InvalidCapability 2
#define seL4_IllegalOperation 3
#define seL4_RangeError 4
#define seL4_AlignmentError 5
#define seL4_FailedLookup 6
#define seL4_TruncatedMessage 7
#define seL4_DeleteFirst 8
#define seL4_RevokeFirst 9
#define seL4_NotEnoughMemory 10
struct seL4_CapRights_t {
  int words[1];
};
#define seL4_ReadWrite seL4_CapRights_t()
#define kprintf printf

static inline int sync_mutex_init(sync_mutex_t* , seL4_Word ){return 0;}

static inline int sync_mutex_lock(sync_mutex_t*){return 0;}
static inline int sync_mutex_unlock(sync_mutex_t*){return 0;}
