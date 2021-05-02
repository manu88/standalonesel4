as -64 crt0.S -o crt0.o
gcc -m64 -static -c m.c -o m.o -nostdlib -ffreestanding -nodefaultlibs  
gcc -m64 -g -static -nostdlib  -fno-pic -fno-pie crt0.o m.o -o program.bin
# Not use now-Wl,-T  layout.lds 
