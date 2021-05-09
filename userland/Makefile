AS = as
CC = gcc
LD = gcc
PROG=program.bin

CRT_OBJS = crti.S crt0.o
OBJS = main.o

LIBS= -L./libsel4/lib/ -lsel4
LDFLAGS= -Wl,-m -Wl,elf_x86_64 -m64 -g -static -nostdlib -fno-stack-protector -fno-pic -fno-pie -Wl,-u_sel4_start -Wl,-e_sel4_start
ASFLAGS= -64
CCFLAGS= -m64 -static -Wall -Wextra -nostdlib -fno-stack-protector
C_INCLUDES= -I./libsel4/include/ -I./libsel4/include/autoconf/ -I./libsel4/include/gen_config/

.S.o:
	$(AS) $(ASFLAGS) $< -o $@

.c.o:
	$(CC) $(C_INCLUDES)  -c  $(CCFLAGS) $<  -o $@

default: $(PROG)

clean:
	rm -f *.o
	rm -f $(PROG)

default: $(PROG)  

$(PROG): $(OBJS) $(CRT_OBJS) crtn.o
	$(LD) $(LDFLAGS) $(CRT_OBJS) /usr/lib/gcc/x86_64-linux-gnu/9/crtbegin.o $(OBJS)  /usr/lib/gcc/x86_64-linux-gnu/9/crtend.o crtn.o -lgcc -L./ -lgcc_eh $(LIBS) -Wl,-T linker.ld -o $(PROG)
