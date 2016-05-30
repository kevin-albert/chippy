include src/Makefile.inc

EXE		= chippy
LIB		= src/app/libapp.a src/util/libutil.a

$(EXE): lib
	$(CXX) $(LDFLAGS) $(LIB) -o $@

.PHONY: lib
lib:
	$(MAKE) -C src/app
	$(MAKE) -C src/util


clean:
	rm -f $(EXE)
	$(MAKE) -C src/app clean
	$(MAKE) -C src/cmp clean
	$(MAKE) -C src/pcm clean
	$(MAKE) -C src/synth clean
	$(MAKE) -C src/ui clean
	$(MAKE) -C src/util clean
	$(MAKE) -C src/views clean
