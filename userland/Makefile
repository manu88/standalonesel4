AS = as
CC = gcc
LD = gcc
PROG=program.bin

OBJS = crt0.o main.o


LDFLAGS= -m64 -g -static -nostdlib  -fno-pic -fno-pie
ASFLAGS= -64
CCFLAGS= -m64 -static

.S.o:
	$(AS) $(ASFLAGS) $< -o $@

.c.o:
	$(CC) -c $(CCFLAGS) $< -o $@

default: $(PROG)

clean:
	rm -f *.o
	rm -f $(PROG)

default: $(PROG)  

$(PROG): $(OBJS)  
	$(LD) $(LDFLAGS) $(OBJS) -o $(PROG)
