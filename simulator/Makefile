#**********************
#*
#* Progam Name: MP1. Membership Protocol.
#*
#* Current file: Makefile
#* About this file: Build Script.
#*
#***********************

CFLAGS =  -Wall -g -std=c++11 -I.. -fsanitize=address -O  -fno-omit-frame-pointer
LDFLAGS = -g -fsanitize=address -lasan
CXX = /usr/local/bin/g++-6
# CXX = g++
# CXX = clang++-3.8

all: simulator

simulator: MP1Node.o EmulNet.o Application.o Log.o Params.o Member.o Trace.o MP2Node.o Node.o HashTable.o Entry.o

MP1Node.o: MP1Node.cpp MP1Node.h Log.h Params.h Member.h emulNet.h Queue.h
	${CXX} -c MP1Node.cpp ${CFLAGS}

EmulNet.o: EmulNet.cpp EmulNet.h Params.h Member.h
	${CXX} -c EmulNet.cpp ${CFLAGS}

Application.o: Application.cpp Application.h Member.h Log.h Params.h Member.h EmulNet.h Queue.h
	${CXX} -c Application.cpp ${CFLAGS}

Log.o: Log.cpp Log.h Params.h Member.h
	${CXX} -c Log.cpp ${CFLAGS}

Params.o: Params.cpp Params.h
	${CXX} -c Params.cpp ${CFLAGS}

Member.o: Member.cpp Member.h
	${CXX} -c Member.cpp ${CFLAGS}

Trace.o: Trace.cpp Trace.h
	${CXX} -c Trace.cpp ${CFLAGS}

MP2Node.o: MP2Node.cpp MP2Node.h EmulNet.h Params.h Member.h Trace.h Node.h HashTable.h Log.h Params.h ../net/Message.h ../net/Transport.h
	${CXX} -c MP2Node.cpp ${CFLAGS}

Node.o: Node.cpp Node.h Member.h
	${CXX} -c Node.cpp ${CFLAGS}

HashTable.o: HashTable.cpp HashTable.h common.h Entry.h
	${CXX} -c HashTable.cpp ${CFLAGS}

Entry.o: Entry.cpp Entry.h
	${CXX} -c Entry.cpp ${CFLAGS}

clean:
	rm -rf *.o
