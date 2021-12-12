CC=g++
LD=g++

OBJS = main.o test_vector.o test_string.o test_expected.o test_optional.o test_vmspace.o VMSpace.o
PROG= unittests


CCFLAGS = -I../ -I../lib/ -g -DUNIT_TESTS

.cpp.o: VMSpace
	$(CC) $(C_INCLUDES) -c  $(CCFLAGS)  $<  -o $@

$(PROG): VMSpace $(OBJS)
	$(LD) $(OBJS) -o $(PROG)

clean:
	rm -f *.o
	rm -f $(PROG)

VMSpace:  
	cp ../VMSpace.cpp .