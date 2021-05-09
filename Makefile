kernelFolder= ./kernel
userland = userland
kernelFile = ./$(userland)/kernel
kernelBuildFolder = ./build
rootServer = ./$(userland)/program.bin

all: kernelFile user

$(kernelBuildFolder):
	mkdir -p $(kernelBuildFolder)

kernel_b: $(kernelBuildFolder) $(kernel)
	
	cd $(kernelBuildFolder) && cmake -DCROSS_COMPILER_PREFIX= -DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake -G Ninja -C ../kernel/configs/X64_verified_test.cmake ..
	cd $(kernelBuildFolder) && ninja kernel.elf

kernelFile: kernel_b
	objcopy -O elf32-i386 $(kernelBuildFolder)/kernel/kernel.elf $(kernelFile)

libsel4: kernel_b
	cp $(kernelBuildFolder)/libsel4/libsel4.a

user:
	cd $(userland) && make

config:
	cp configs/X64_verified_test.cmake $(kernelFolder)/configs/

update:
	sh update.sh

clean:
	rm -rf $(kernelBuildFolder)
	cd $(userland) && make clean
	rm -f $(kernelFile)

sim:
	qemu-system-x86_64  -cpu Nehalem,-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,enforce \
	-nographic -serial mon:stdio -kernel $(kernelFile) -initrd $(rootServer)