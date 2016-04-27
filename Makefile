CFLAGS=-O2
CXXFLAGS=${CFLAGS} -std=c++11

all: deckeval

deckeval: file.o
	@#

file.o: file.cc file.h
