#as --32 multi.S -o multi.o


#compile kernel.c file
gcc -static -m64 -c m.c -o m.o -nostdlib -ffreestanding -nodefaultlibs -fno-stack-protector -fno-builtin 
#gcc -m32 -c main.c -o main.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra

#linking the kernel with kernel.o and boot.o files
ld  -T linker.ld m.o -o program.bin -nostdlib

#grub-file --is-x86-multiboot program.bin