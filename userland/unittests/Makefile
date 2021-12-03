CC=g++
LD=g++

OBJS = main.o test_vector.o
PROG= unittests


CCFLAGS = -I../lib/

.cpp.o:
	$(CC) $(C_INCLUDES) -c  $(CCFLAGS)  $<  -o $@

$(PROG): $(OBJS)
	$(LD) $(OBJS) -o $(PROG)