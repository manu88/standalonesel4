#pragma once
#include <stdint.h>
#include "../sel4.hpp"


#define SEL4_TIME_UINT_TYPE(v)  UINT64_C(v)
#define NS_IN_US        SEL4_TIME_UINT_TYPE(1000)
#define US_IN_MS        SEL4_TIME_UINT_TYPE(1000)
#define MS_IN_S         SEL4_TIME_UINT_TYPE(1000)
#define US_IN_S         (US_IN_MS * MS_IN_S)
#define NS_IN_S         (NS_IN_US * US_IN_S)

class PlatformExpert;
class ObjectFactory;
class PIT{
public:
    bool init(PlatformExpert &expert, ObjectFactory *factory);
    bool setTimeout(uint64_t ns, bool periodic);
    seL4_SlotPos irqNotif = 0;
    seL4_SlotPos irqCap = 0;
private:
    bool configure(uint8_t mode, uint64_t ns);
    PlatformExpert *_expert = nullptr;
    seL4_SlotPos _portCap = 0;
};