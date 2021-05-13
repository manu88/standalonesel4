AS = as
CC = gcc
LD = gcc
PROG=program.bin

CRT_OBJS = crti.o crt0.o crtn.o
OBJS = main.o


LDFLAGS= -Wl,-m -Wl,elf_x86_64 -m64 -g -static -nostdlib  -fno-pic -fno-pie -Wl,-u_sel4_start -Wl,-e_sel4_start
ASFLAGS= -64
CCFLAGS= -m64 -static -Wall -Wextra

.S.o:
	$(AS) $(ASFLAGS) $< -o $@

.c.o:
	$(CC) -c $(CCFLAGS) $< -o $@

default: $(PROG)

clean:
	rm -f *.o
	rm -f $(PROG)

default: $(PROG)  

$(PROG): $(OBJS) $(CRT_OBJS)  
	$(LD) $(LDFLAGS) crti.o /usr/lib/gcc/x86_64-linux-gnu/9/crtbegin.o crt0.o $(OBJS)  /usr/lib/gcc/x86_64-linux-gnu/9/crtend.o crtn.o -lgcc -lgcc_eh  -Wl,-T linker.ld -o $(PROG)
