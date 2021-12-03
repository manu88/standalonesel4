AS=as
CC=g++
LD=g++
PROG=program.bin

CRT_OBJS = crti.o crt0.o
OBJS = main.o printf.o putchar_impl.o init.o stdcpp_stubs.o tls.o cstubs.o sel4.o InitialUntypedPool.o PageTable.o RootServer.o VMSpace.o Thread.o ObjectFactory.o kmalloc.o Shell.o

LIBS= -L./libsel4/lib/ -lsel4
CPP_FLAGS=-fno-rtti -fno-exceptions -std=c++2a
LDFLAGS= -Wl,-m -Wl,elf_x86_64 -g -static $(CPP_FLAGS) -fno-builtin -nostdlib -fno-stack-protector -fno-pic -fno-pie -Wl,-u_sel4_start -Wl,-e_sel4_start
ASFLAGS= -64
CCFLAGS= -m64 -static -Wall -Wextra -nostdlib -fno-stack-protector -fno-builtin $(CPP_FLAGS) 

C_INCLUDES= -I./libsel4/include/ -I./libsel4/include/autoconf/ -I./libsel4/include/gen_config/

.S.o:
	$(AS) $(ASFLAGS) $< -o $@

.c.o:
	$(CC) $(C_INCLUDES) -c $(CCFLAGS)  $<  -o $@

.cpp.o:
	$(CC) $(C_INCLUDES) -c  $(CCFLAGS)  $<  -o $@

default: $(PROG)

clean:
	rm -f *.o
	rm -f $(PROG)

default: $(PROG)  

$(PROG): $(OBJS) $(CRT_OBJS) crtn.o
	$(LD) -Wl,-T linker.ld $(LDFLAGS)  crti.o crt0.o /usr/lib/gcc/x86_64-linux-gnu/9/crtbegin.o $(OBJS)  /usr/lib/gcc/x86_64-linux-gnu/9/crtend.o crtn.o -lgcc -L./ -lgcc_eh $(LIBS) -o $(PROG)
