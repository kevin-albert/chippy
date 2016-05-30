CC			= g++
CXX			= $(CC)
STD         = c++14
CPPFLAGS	= -std=$(STD) -Ofast -c -I..


UNAME		= $(shell uname -s)
ifeq	($(UNAME),Linux)
	LDFLAGS += -lasound
endif
