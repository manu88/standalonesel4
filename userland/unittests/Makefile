CC=g++
LD=g++

OBJS = main.o test_vector.o
PROG= unittests


CCFLAGS = -I../lib/ -g -DUNIT_TESTS

.cpp.o:
	$(CC) $(C_INCLUDES) -c  $(CCFLAGS)  $<  -o $@

$(PROG): $(OBJS)
	$(LD) $(OBJS) -o $(PROG)

clean:
	rm -f *.o
	rm -f $(PROG)