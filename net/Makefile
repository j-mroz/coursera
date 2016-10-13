CFLAGS =  -Wall -g -std=c++11 -I.. -fsanitize=address -O0  -fno-omit-frame-pointer
LDFLAGS = -g -fsanitize=address -lasan -fsanitize=address -O0
# CXX = /usr/local/bin/g++-6
# CXX = g++
CXX = clang++-3.8


all: Transport.o

Transport.o: Transport.cpp Transport.h ../simulator/Member.h ../simulator/EmulNet.h ../simulator/Queue.h
	${CXX} -c Transport.cpp ${CFLAGS}

clean:
	rm -rf *.o
