cmake_minimum_required(VERSION 3.7.2)

list(
    APPEND
        CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/kernel/
)

enable_language(C)
enable_language(CXX)
enable_language(ASM)




add_subdirectory("kernel" kernel)
include(${KERNEL_HELPERS_PATH})

add_subdirectory("kernel/libsel4" libsel4)