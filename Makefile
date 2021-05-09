kernelFolder= ./kernel
userland = userland
kernelFile = ./$(userland)/kernel
kernelBuildFolder = ./build
rootServer = ./$(userland)/program.bin
ARCH=x86_64
all: kernelFile user

$(kernelBuildFolder):
	mkdir -p $(kernelBuildFolder)

kernel_b: $(kernelBuildFolder) $(kernel)
	cd $(kernelBuildFolder) && cmake -DCROSS_COMPILER_PREFIX= -DCMAKE_TOOLCHAIN_FILE=../kernel/gcc.cmake -G Ninja -C ../kernel/configs/X64_verified_test.cmake ..
	cd $(kernelBuildFolder) && ninja kernel.elf
	cd $(kernelBuildFolder) && ninja libsel4.a

kernelFile: kernel_b
	objcopy -O elf32-i386 $(kernelBuildFolder)/kernel/kernel.elf $(kernelFile)

libsel4: kernel_b
	mkdir -p $(userland)/libsel4
	mkdir -p $(userland)/libsel4/lib/
	cp $(kernelBuildFolder)/libsel4/libsel4.a $(userland)/libsel4/lib/
	cp -r $(kernelFolder)/libsel4/include $(userland)/libsel4/
	cp -r  $(kernelBuildFolder)/libsel4/autoconf/ $(userland)/libsel4/include/
	mkdir -p $(userland)/libsel4/include/gen_config
	cp -r $(kernelBuildFolder)/kernel/gen_config/kernel/ $(userland)/libsel4/include/gen_config
	cp -r $(kernelBuildFolder)/libsel4/gen_config/sel4/ $(userland)/libsel4/include/gen_config

	cp $(kernelBuildFolder)/libsel4/include/sel4/* $(userland)/libsel4/include/sel4/
	cp -r $(kernelFolder)/libsel4/sel4_arch_include/$(ARCH)/sel4/sel4_arch/ $(userland)/libsel4/include/sel4/
	cp $(kernelBuildFolder)/libsel4/sel4_arch_include/$(ARCH)/sel4/sel4_arch/* $(userland)/libsel4/include/sel4/sel4_arch/

	cp -r $(kernelFolder)/libsel4/arch_include/x86/sel4/arch/ $(userland)/libsel4/include/sel4/

	cp -r kernel/libsel4/mode_include/64/sel4/mode $(userland)/libsel4/include/sel4/

user: libsel4
	cd $(userland) && make

config:
	cp configs/X64_verified_test.cmake $(kernelFolder)/configs/

update:
	sh update.sh

clean:
	rm -rf $(kernelBuildFolder)
	rm -r $(userland)/libsel4/ 
	cd $(userland) && make clean
	rm -f $(kernelFile)

sim:
	qemu-system-x86_64  -cpu Nehalem,-vme,+pdpe1gb,-xsave,-xsaveopt,-xsavec,-fsgsbase,-invpcid,enforce \
	-nographic -m 256 -serial mon:stdio -kernel $(kernelFile) -initrd $(rootServer)