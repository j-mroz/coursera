#**********************
#*
#* Progam Name: MP1. Membership Protocol.
#*
#* Current file: Makefile
#* About this file: Build Script.
#*
#***********************

CFLAGS =  -Wall -g -std=c++11 -fsanitize=address -O0  -fno-omit-frame-pointer
LDFLAGS = -g  -lasan -fsanitize=address
CXX = /usr/local/bin/g++-6
# CXX = g++
# CXX = clang++-3.8

OBJS= $(shell find . -name "*.o" -print)

all:
	$(MAKE) -j8 -C simulator
	$(MAKE) -j8 -C net
	$(MAKE) -j8 -C service
	${CXX} -o Application simulator/*.o net/*.o service/*.o ${CFLAGS} ${LDFLAGS}

clean:
	$(MAKE) clean -C simulator
	$(MAKE) clean -C net
	$(MAKE) clean -C service
	rm -rf *.o Application dbg.log msgcount.log stats.log machine.log *.dSYM .DS_Store
