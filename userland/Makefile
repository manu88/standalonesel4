AS = as
CC = gcc
LD = gcc
PROG=program.bin

CRT_OBJS = crti.S crt0.o
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
	$(LD) $(LDFLAGS) $(CRT_OBJS) /usr/lib/gcc/x86_64-linux-gnu/9/crtbegin.o $(OBJS)  /usr/lib/gcc/x86_64-linux-gnu/9/crtend.o /home/manu/dev/sel4/Sofa/build/lib/crtn.o -lgcc -lgcc_eh  -Wl,-T linker.ld -o $(PROG)
