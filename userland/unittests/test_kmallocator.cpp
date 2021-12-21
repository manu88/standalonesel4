#include "tests.hpp"
#include "kmalloc.hpp"
#include <cstdlib>
#include <assert.h>
//#include "vector.hpp"

void testKmallocator(){
    printf("testKmallocator Base:\n");
    assert(kfree != free);
    size_t memSize = 4096*10;
    void* memPtr = malloc(memSize);
    assert(memPtr);
    setMemoryPool(memPtr, memSize, 0);
    assert(getTotalKMallocated() == 0);
    char* ptr= (char*) kmalloc(1024); 
    assert(ptr);
    for (int i=0;i<1024;i++){
        ptr[i] = 'c';
    }
    printf("getTotalKMallocated()=%zi\n", getTotalKMallocated());
    assert(getTotalKMallocated() == 1024);
    kfree(ptr);
    assert(getTotalKMallocated() == 0);

    printf("testKmallocator krealloc:\n");
    int* ptr2 = (int*) krealloc(nullptr, 512);
    assert(ptr2);
    for (int i=0;i<2048;i++){
        ptr2[i] = i;
    }
    printf("Test krealloc\n");
    int* ptr22 = (int*) krealloc(ptr2, 1024);
    printf("Test krealloc OK\n");
    assert(ptr22);
    for (int i=0;i<2048;i++){
        assert(ptr22[i] == i);
    }
}