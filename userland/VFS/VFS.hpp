#pragma once

struct BlockDevice;

class VFS{
public:
    bool init();
    bool inpectDev(BlockDevice&);
};