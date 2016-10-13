#!/bin/sh
rm -f *.h *.tcc *.cpp
thrift -out . -r -gen cpp:templates dht_proto.thrift
