#pragma once
#include "sel4.hpp"
#include <stdint.h>

class Process{
public:

/* sel4utils processes start with some caps in their cspace.
 * These are the caps
 * see https://github.com/seL4/seL4_libs/blob/master/libsel4utils/include/sel4utils/process.h
 */
enum class CSpaceLayout: seL4_Word {
    /* no cap in NULL */
    NULL_SLOT = 0,
    /*
     * The root cnode (with appropriate guard)
     */
    CNODE_SLOT = 1,
    /* The slot on the cspace that fault_endpoint is put if
     * sel4utils_configure_process is used.
     */
    ENDPOINT_SLOT = 2,

    /* The page directory slot */
    PD_SLOT = 3,

    /* the slot for the asid pool that this thread is in and can create threads
     * in. 0 if this kernel does not support asid pools */
    ASID_POOL_SLOT = 4,

    /* the slot for this processes tcb */
    TCB_SLOT = 5,

    /* the slot for this processes sc */
    SCHED_CONTEXT_SLOT = 6,

    /* The slot for this processes reply object */
    REPLY_SLOT = 7,

    /* First free slot in the cspace configured by sel4utils */
    FIRST_FREE = 8
};

seL4_Word cspace;
uint32_t cspaceSize = 12;
seL4_Word pageDir;

private:
};