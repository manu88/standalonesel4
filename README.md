## Sel4 standalone test
This repo is a test for building a seL4 rootserver without using the toolchain and other librairies offered by seL4.

The why and the outcome of this experiment remains - even to the written -  unknown :).

Maybe for science purposes.


# steps
```bash
make update
make #Might fail the 1st time, for some capDL err. re-issue command.
make sim
```
## Useful links:
* [genode seL4 experiment](https://genode.org/documentation/articles/sel4_part_1)
* [genode seL4 related code](https://github.com/genodelabs/genode/tree/master/repos/base-sel4)

## Dist: not working
```bash
sh update.sh
# build kernel
cp configs/X64_verified_test.cmake kernel/configs/
mkdir build
cd build
cmake -DCROSS_COMPILER_PREFIX= -DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake -G Ninja -C ../kernel/configs/X64_verified_test.cmake ../kernel/
ninja kernel.elf
# create disk image
cd ../dist/
objcopy -O elf32-i386 ../build/kernel.elf kernel
sudo sh build.sh sofa.img mntP/
# start in QEMU
sh start.sh
```