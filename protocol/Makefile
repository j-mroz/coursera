CFLAGS =  -Wall -g -std=c++11 -I. -I.. -fsanitize=address -O0   -fno-omit-frame-pointer
LDFLAGS = -g -fsanitize=address -lasan -fsanitize=address -O0 
# CXX = /usr/local/bin/g++-6
# CXX = g++
CXX = clang++-3.8


all: dht_proto.o

dht_proto.o: dht_proto_constants.cpp dht_proto_constants.h dht_proto_types.cpp dht_proto_types.h dht_proto_types.tcc
	${CXX} -c dht_proto_types.cpp dht_proto_constants.cpp ${CFLAGS}

clean:
	rm -rf *.o
