CFLAGS=-Os
CXXFLAGS=${CFLAGS} -std=c++11

all: deckeval tests

deckeval: file.o mapping.o json.o carddb.o
	@#

tests: tests.o file.o mapping.o json.o carddb.o game.o
	${CXX} ${CXXFLAGS} -o tests $^

tests.o: tests.cc carddb.h file.h mapping.h json.h
file.o: file.cc file.h
mapping.o: mapping.cc mapping.h file.h
json.o: json.cc json.h mapping.h
	${CXX} ${CXXFLAGS} -O3 -c -o json.o $<
carddb.o: carddb.cc carddb.h json.h mapping.h file.h
