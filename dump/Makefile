
CFLAGS=-g -Wall -Werror

TARGETS=map

all: $(TARGETS)

map: map.cpp
	g++ -std=c++11 -o map map.cpp

clean:
	rm -f $(TARGETS) 
	rm -rf *.dSYM

distclean: clean
