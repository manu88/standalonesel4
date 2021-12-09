#include <cstdint>
#include "runtime.h"
#include "sel4.hpp"

// Minimum alignment across all platforms.
#define MIN_ALIGN_BYTES 16
#define MIN_ALIGNED __attribute__((aligned (MIN_ALIGN_BYTES)))
#define CONFIG_SEL4RUNTIME_STATIC_TLS 16384

static char static_tls[CONFIG_SEL4RUNTIME_STATIC_TLS] MIN_ALIGNED = {};

extern "C" void init_tls()
{
    seL4_TCB_SetTLSBase(seL4_CapInitThreadTCB, (seL4_Word)static_tls);
    //seL4_SetTLSBase((seL4_Word)static_tls);
}   
