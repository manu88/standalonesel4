#include <cstdint>
#include "runtime.h"
#include "sel4.hpp"
// Minimum alignment across all platforms.
#define MIN_ALIGN_BYTES 16
#define MIN_ALIGNED __attribute__((aligned (MIN_ALIGN_BYTES)))
#define CONFIG_SEL4RUNTIME_STATIC_TLS 16384
extern "C"
{

extern unsigned int _tdata_start[];
extern unsigned int _tdata_end[];
extern unsigned int _tbss_end[];


static char static_tls[CONFIG_SEL4RUNTIME_STATIC_TLS] MIN_ALIGNED = {};

#if 0
uintptr_t sel4runtime_write_tls_image(void *tls_memory)
{
    if (tls_memory == SEL4RUNTIME_NULL) {
        return (uintptr_t)SEL4RUNTIME_NULL;
    }

    copy_tls_data(tls_memory);

    return tls_base_from_tls_region(tls_memory);
}

void sel4runtime_move_initial_tls(void *tls_memory)
{
    if (tls_memory == nullptr) {
        return;
    }

    uintptr_t tls_base = sel4runtime_write_tls_image(tls_memory);
    if (tls_base == (uintptr_t)nullptr) {
        return;
    }

    sel4runtime_set_tls_base(tls_base);
}
#endif
void init_tls()
{
    //sel4runtime_move_initial_tls(static_tls);
    printf("_tdata_start is at %p\n", (void*)_tdata_start);
    printf("_tdata_end is at %p\n", (void*)_tdata_end);
    printf("_tbss_end is at %p\n", (void*)_tbss_end);
    seL4_SetTLSBase((seL4_Word)static_tls);
    printf("SetTLSBase OK\n");
}   

}