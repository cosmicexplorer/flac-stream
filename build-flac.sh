#!/bin/sh

cd "$1"
./autogen.sh && ./configure && make
