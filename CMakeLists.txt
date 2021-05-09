cmake_minimum_required(VERSION 3.7.2)

project(Sofa)

list(
    APPEND
        CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/helpers/ ${CMAKE_CURRENT_LIST_DIR}/configs/
)

enable_language(C)
enable_language(CXX)
enable_language(ASM)


find_file(KERNEL_PATH kernel PATHS ${CMAKE_SOURCE_DIR} NO_CMAKE_FIND_ROOT_PATH)

add_subdirectory("${KERNEL_PATH}" kernel)

set(Debug "1" CACHE STRING "")
add_subdirectory("${KERNEL_PATH}/libsel4" libsel4)