// from https://github.com/seL4/util_libs/blob/master/libplatsupport/src/plat/pc99/pit.c
#include "Pit.hpp"
#include "../klog.h"
#include <stdint.h>
#include "PlatformExpert.hpp"
#include "../ObjectFactory.hpp"
#include "../runtime.h"


#define PITCR_MODE_ONESHOT   0x0
#define PITCR_MODE_PERIODIC  0x2


#define TICKS_PER_SECOND 1193182
#define PIT_TICKS_TO_NS(ticks) ((uint32_t)(ticks) * (NS_IN_S / TICKS_PER_SECOND))
#define PIT_MIN_TICKS 2
#define PIT_MAX_TICKS 0xFFFF
#define PIT_MIN_NS PIT_TICKS_TO_NS(PIT_MIN_TICKS)
#define PIT_MAX_NS PIT_TICKS_TO_NS(PIT_MAX_TICKS)


#define PIT_NS_TO_TICKS(ns) ((ns) * TICKS_PER_SECOND / NS_IN_S)

#define PIT_IOPORT_CHANNEL(x) (0x40 + x) /* valid channels are 0, 1, 2. we'll be using 0 exclusively, though */
#define PIT_IOPORT_COMMAND  0x43
#define PIT_IOPORT_PITCR    PIT_IOPORT_COMMAND

#define PITCR_SET_CHANNEL(x, channel)   (((channel) << 6) | x)
#define PITCR_SET_OP_MODE(x, mode)      (((mode) << 1)    | x)
#define PITCR_SET_ACCESS_MODE(x, mode)  (((mode) << 4)    | x)

#define PITCR_ACCESS_LOHI  0x3

bool PIT::init(PlatformExpert &expert, ObjectFactory *factory){
    _expert = &expert;
    auto slotOrerr = expert.issuePortRangeWithSize(PIT_IOPORT_CHANNEL(0), 4);
    if(!slotOrerr){
        return false;
    }
    _portCap = slotOrerr.value;

    auto notifOrErr = factory->createNotification();
    if(!notifOrErr){
        factory->releaseSlot(irqCap);
        return false;
    }
    irqNotif = notifOrErr.value;

    return true;
}


bool PIT::setTimeout(uint64_t ns, bool periodic){
    uint32_t mode = periodic ? PITCR_MODE_PERIODIC : PITCR_MODE_ONESHOT;
    return configure(mode, ns);
}

/* helper functions */
static inline seL4_Error
setPitMode(seL4_SlotPos portCap, uint8_t channel, uint8_t mode){
    auto ret = seL4_X86_IOPort_Out8(portCap, PIT_IOPORT_PITCR, PITCR_SET_CHANNEL(PITCR_SET_OP_MODE(PITCR_SET_ACCESS_MODE(0, PITCR_ACCESS_LOHI), mode), channel));
    return ret;
}

bool PIT::configure(uint8_t mode, uint64_t ns){
    if (ns > PIT_MAX_NS || ns < PIT_MIN_NS) {
        kprintf("ns invalid for programming PIT %zu <= %zu <= %zu\n",
                (uint64_t)PIT_MIN_NS, ns, (uint64_t)PIT_MAX_NS);
        return false;
    }

    /* configure correct mode */
    auto error = setPitMode(_portCap, 0, mode);
    if (error != seL4_NoError) {
        kprintf("ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return false;
    }

    uint64_t ticks = PIT_NS_TO_TICKS(ns);
    /* program timeout */
    error = seL4_X86_IOPort_Out8(_portCap, PIT_IOPORT_CHANNEL(0), (uint8_t) (ticks & 0xFF));
    if (error != seL4_NoError) {
        kprintf("seL4_X86_IOPort_Out8 failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return false;
    }

    error = seL4_X86_IOPort_Out8(_portCap, PIT_IOPORT_CHANNEL(0), (uint8_t) (ticks >> 8) & 0xFF);
    if (error != seL4_NoError) {
        kprintf("ps_io_port_out failed on channel %u\n", PIT_IOPORT_CHANNEL(0));
        return false;
    }

    auto irqHandleOrErr = _expert->getIOAPICIRQHandle(0, 42, 2);
    if(!irqHandleOrErr){
        return false;
    }
    irqCap = irqHandleOrErr.value.irqCap;
    irqNotif = irqHandleOrErr.value.notif;
    seL4_IRQHandler_Ack(irqCap);
    return true;
}