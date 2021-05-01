# steps

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
sudo sh build.sh sofa.img mntP/
# start in QEMU
sh start.sh
```