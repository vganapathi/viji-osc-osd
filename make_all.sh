#!/bin/sh

cd kernel
make $@ 
cd ../util
make $2 || exit 1
cd ../osd_initiator
make $@ || exit 1
cd ./python
make $@ || exit 1
cd ..
cd ../osd-target
make $@ 
