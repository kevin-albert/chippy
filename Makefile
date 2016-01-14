CC		= g++
CFLAGS	= -std=c++11 -Ofast -c
LDFLAGS	= -ldl -lcurses
SRC		= main.cpp dj_view.cpp sequencer_view.cpp mixer_view.cpp ui.cpp \
		  controller.cpp project.cpp synth_api.cpp pcm_wrapper.cpp expr.cpp util.cpp
OBJ		= $(SRC:.cpp=.o)
UNAME	= $(shell uname -s)
EXE		= chippy

ifeq	($(UNAME),Linux)
	LDFLAGS += -lasound
else
	LDFLAGS += -L/usr/local/lib -lportaudio
	CFLAGS += -I/usr/local/include
endif

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

install: $(EXE)
	mkdir -p /opt/chippy_files
	cp chippy /usr/local/bin

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
	rm -rf *.o $(EXE) wav_test project_debug Makefile.deps

Makefile.deps:
	$(CC) -std=c++11 -MM $(SRC) >Makefile.deps

