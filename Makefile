CC		= g++
CFLAGS	= -std=c++11 -Ofast -c
LDFLAGS	= -ldl -lcurses
SRC		= main.cpp controller.cpp project.cpp synth_api.cpp pcm_wrapper.cpp expr.cpp util.cpp
OBJ		= $(SRC:.cpp=.o)
UNAME	= $(shell uname -s)

ifeq	($(UNAME),Linux)
	LDFLAGS += -lasound
endif

chippy: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

wav_test: pcm_wrapper.cpp util.h
	$(CC) -DWAV_TEST -std=c++11 $< -o $@

project_debug: project_debug.cpp project.o
	$(CC) -std=c++11 $^ -o $@

deploy:
	git push device master

$(OBJ): %.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

include Makefile.deps

clean:
	rm -rf *.o chippy Makefile.deps

Makefile.deps:
	$(CC) -std=c++11 -MM $(SRC) >Makefile.deps

