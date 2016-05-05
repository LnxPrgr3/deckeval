CFLAGS=-Os
CXXFLAGS=${CFLAGS} -std=c++11

all: deckeval

deckeval: file.o mapping.o json.o carddb.o
	@#

file.o: file.cc file.h
mapping.o: mapping.cc mapping.h file.h
json.o: json.cc json.h mapping.h
	${CXX} ${CXXFLAGS} -O3 -c -o json.o $<
carddb.o: carddb.cc carddb.h json.h mapping.h file.h
