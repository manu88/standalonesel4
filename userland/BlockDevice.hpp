#pragma once
#include <sys/types.h> // ssize_t

struct BlockDevice{
    virtual ~BlockDevice(){}

    virtual ssize_t read(size_t sector, char* buf, size_t bufSize) {return -1;}
};