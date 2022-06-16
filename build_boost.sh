#!/bin/bash
machine=`uname -m|grep '64'`

if [ a"$machine" == "a" ]; then
   bitlevel=32
else
   bitlevel=64
fi
cd opensource/boost
./bootstrap.sh --prefix=./
#./bjam  cxxflags='-fPIC -std=c++11' link=static threading=multi variant=release address-model=$bitlevel toolset=gcc runtime-link=static
./b2 -a  cxxflags='-fPIC -std=c++11' install --prefix=/home/zkm/emeraldb/opensource/boost/stage link=static threading=multi boost.locale.icu=off