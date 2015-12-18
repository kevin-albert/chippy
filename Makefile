CC		= g++
CFLAGS	= -std=c++11 -Ofast -c
LDFLAGS	= 
SRC		= test.cpp expr.cpp
OBJ		= $(SRC:.cpp=.o)

test: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ): %.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

include Makefile.deps

clean:
	rm -rf *.o test Makefile.deps

Makefile.deps:
	$(CC) -MM $(SRC) >Makefile.deps


