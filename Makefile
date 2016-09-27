#**********************
#*
#* Progam Name: MP1. Membership Protocol.
#*
#* Current file: Makefile
#* About this file: Build Script.
#*
#***********************

CFLAGS =  -Wall -g -std=c++11 -fsanitize=address -O  -fno-omit-frame-pointer
LDFLAGS = -g -fsanitize=address -lasan
CXX = /usr/local/bin/g++-6
# CXX = g++
# CXX = clang-3.8

OBJS = $(shell find . -name "*.o" -print)

all:
	$(MAKE) -C simulator
	$(MAKE) -C net
	${CXX} -o Application ${OBJS} ${CFLAGS}


clean:
	$(MAKE) clean -C simulator
	$(MAKE) clean -C net
	rm -rf *.o Application dbg.log msgcount.log stats.log machine.log
