CXX=gcc
CFLAGS=-Wall -pedantic -O3
LIBS=-lncurses -lmenu

Apocalypse: main.c menu_strings.h text_strings.h
	$(CXX) $(CFLAGS) main.c -o $@ $(LIBS)

run: Apocalypse
	./Apocalypse

clean:
	rm Apocalypse

.PHONY: run clean
