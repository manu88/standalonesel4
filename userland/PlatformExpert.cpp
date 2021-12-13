#include "PlatformExpert.hpp"
#include "runtime.h"

bool PlatformExpert::init(){
    kprintf("PlatformExpert::init for x86_64\n");
    return true;
}