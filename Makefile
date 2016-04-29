CFLAGS=-O2
CXXFLAGS=${CFLAGS} -std=c++11

all: deckeval

deckeval: file.o mapping.o json.o carddb.o
	@#

file.o: file.cc file.h
mapping.o: mapping.cc mapping.h file.h
json.o: json.h mapping.h
carddb.o: carddb.h json.h mapping.h file.h
