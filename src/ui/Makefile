include ../Makefile.inc

SRC			= util.cpp

libutil.a: libutil.a(util.o)
util.o: util.cpp

ifneq ($(MAKECMDGOALS),clean)
include Makefile.deps
Makefile.deps:
	$(CC) -std=c++14 -MM $(SRC) >Makefile.deps
endif

clean:
	rm -f *.o *.a
