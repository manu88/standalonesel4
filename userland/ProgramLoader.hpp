#pragma once
#include <stddef.h>


class ProgramLoader{
public:
    bool load(const void* data, size_t dataSize);
};