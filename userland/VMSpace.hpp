#pragma once
#include "sel4.hpp"


struct VMSpace
{
private:
    seL4_CPtr _vspace = 0;
};