include ../Makefile.inc

SRC			= main.cpp

libapp.a: libapp.a(app.o)
app.o: app.cpp

ifneq ($(MAKECMDGOALS),clean)
include Makefile.deps
Makefile.deps:
	$(CC) -std=c++14 -I.. -MM $(SRC) >Makefile.deps
endif

clean:
	rm -f *.o main
