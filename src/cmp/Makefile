include ../Makefile.inc

SRC			= cmp.cpp

libcmp.a: libcmp.a(cmp.o)
cmp.o: cmp.cpp

include Makefile.deps

Makefile.deps:
	$(CC) -std=c++14 -MM $(SRC) >Makefile.deps

clean:
	rm -f *.o *.a
