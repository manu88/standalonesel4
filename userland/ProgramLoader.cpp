#include "ProgramLoader.hpp"
#include "elf.h"
#include "klog.h"

static bool sectionIsLoadable(const elf_t *elfFile, int index)
{
    return elf_getProgramHeaderType(elfFile, index) == PT_LOAD;
}

static int countLoadableRegions(const elf_t *elfFile)
{
    int num_headers = elf_getNumProgramHeaders(elfFile);
    int loadable_headers = 0;

    for (int i = 0; i < num_headers; i++) {
        /* Skip non-loadable segments (such as debugging data). */
        if (sectionIsLoadable(elfFile, i)) {
            loadable_headers++;
        }
    }
    return loadable_headers;
}

bool ProgramLoader::load(const void* data, size_t dataSize){
    elf_t file;
    if(elf_newFile(data, dataSize, &file) != 0){
        kprintf("elf_newFile error\n");
        return false;
    }
    int numHeaders = elf_getNumProgramHeaders(&file);
    int numLoadableRegions = countLoadableRegions(&file);
    //size_t numSections = elf_getNumSections(&file);
    kprintf("ELF File is valid and has %i headers and %zi regions to load\n", numHeaders, numLoadableRegions);
    uint64_t entryPoint = elf_getEntryPoint(&file);
    kprintf("Program entry point is at 0X%X\n", entryPoint);

    for (int index = 0; index < numHeaders; index++) {
        auto type = elf_getProgramHeaderType(&file, index);
        auto offset =  elf_getProgramHeaderOffset(&file, index);
        uintptr_t vaddr = elf_getProgramHeaderVaddr(&file, index);
        uintptr_t paddr = elf_getProgramHeaderPaddr(&file, index);
        kprintf("type=0X%X offset=0X%X vaddr=0X%X paddr=0X%X\n", type, offset, vaddr, paddr);
    }
    return true;
}