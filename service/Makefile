CFLAGS =  -Wall -g -std=c++11 -I. -I.. -fsanitize=address -O0  -fno-omit-frame-pointer
LDFLAGS = -g -fsanitize=address -lasan -fsanitize=address -O0 
# CXX = /usr/local/bin/g++-6
# CXX = g++
CXX = clang++-3.8


all: DistributedHashTable.o

DistributedHashTable.o: DistributedHashTable.h src/RingDHT.cpp
	${CXX} -c src/RingDHT.cpp ${CFLAGS} -o DistributedHashTable.o

clean:
	rm -rf *.o
