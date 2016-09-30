CFLAGS =  -Wall -g -std=c++11 -I.. -fsanitize=address -O  -fno-omit-frame-pointer
LDFLAGS = -g -fsanitize=address -lasan
CXX = /usr/local/bin/g++-6
# CXX = g++
# CXX = clang-3.8


all: Message.o Transport.o

Message.o: Message.h Address.h
	${CXX} -c Message.cpp ${CFLAGS} -Inet

Transport.o: Transport.cpp Transport.h ../simulator/Member.h ../simulator/EmulNet.h ../simulator/Queue.h
	${CXX} -c Transport.cpp ${CFLAGS}

clean:
	rm -rf *.o
