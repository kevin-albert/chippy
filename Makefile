CC		= g++
CFLAGS	= -std=c++11 -Ofast -c
LDFLAGS	= -ldl
SRC		= main.cpp controller.cpp expr.cpp pcm_wrapper.cpp pattern.cpp
OBJ		= $(SRC:.cpp=.o)
UNAME	= $(shell uname -s)

ifeq	($(UNAME),Linux)
	LDFLAGS += -lasound
endif

test: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ): %.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

include Makefile.deps

clean:
	rm -rf *.o test Makefile.deps

Makefile.deps:
	$(CC) -std=c++11 -MM $(SRC) >Makefile.deps


