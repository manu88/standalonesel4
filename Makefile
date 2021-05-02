kernelFolder= ./kernel
userland = userland
kernelFile = ./$(userland)/kernel
kernelBuildFolder = ./kernel_build
rootServer = ./$(userland)/program.bin

all: kernelFile user

$(kernelBuildFolder):
	mkdir -p $(kernelBuildFolder)

kernel_b: $(kernelBuildFolder) $(kernel)
	
	cd $(kernelBuildFolder) && cmake -DCROSS_COMPILER_PREFIX= -DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake -G Ninja -C ../kernel/configs/X64_verified_test.cmake ../kernel/
	cd $(kernelBuildFolder) && ninja kernel.elf

kernelFile: kernel_b
	objcopy -O elf32-i386 kernel_build/kernel.elf $(kernelFile)

user:
	cd $(userland) && make

update:
	sh update.sh

clean:
	rm -rf $(kernelBuildFolder)
	cd $(userland) && make clean
	rm -f $(kernelFile)

sim:
	qemu-system-x86_64  -cpu Nehalem,-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,enforce \
	-nographic -serial mon:stdio -kernel $(kernelFile) -initrd $(rootServer)
