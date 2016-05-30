include ../Makefile.inc

SRC			= pcm.cpp

libpcm.a: libpcm.a(pcm.o)
pcm.o: pcm.cpp

ifneq ($(MAKECMDGOALS),clean)
include Makefile.deps
Makefile.deps:
	$(CC) -std=c++14 -MM $(SRC) >Makefile.deps
endif

clean:
	rm -f *.o *.a
