CC=g++
LD=g++

OBJS = main.o test_vector.o test_string.o test_expected.o test_optional.o test_vmspace.o test_kmallocator.o VMSpace.o kmalloc.o liballoc.o
PROG= unittests


CCFLAGS = -I./ -I../ -I../lib/ -g -DUNIT_TESTS

.cpp.o: VMSpace Kmalloc
	$(CC) $(C_INCLUDES) -c  $(CCFLAGS)  $<  -o $@

.c.o: Kmalloc
	$(CC) $(C_INCLUDES) -c  $(CCFLAGS)  $<  -o $@

$(PROG): VMSpace Kmalloc $(OBJS)
	$(LD) $(OBJS) -o $(PROG)

clean:
	rm -f *.o
	rm -f $(PROG)

VMSpace:  
	cp ../VMSpace.cpp .

Kmalloc:
	cp ../kmalloc.cpp .
	cp ../liballoc.c .