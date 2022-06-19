#!/bin/bash
machine=`uname -m|grep '64'`

if [ a"$machine" == "a" ]; then
   bitlevel=32
else
   bitlevel=64
fi


echo ${CURRENT_DIR}
cd opensource/boost
./bootstrap.sh
#./bjam  cxxflags='-fPIC -std=c++11' link=static threading=multi variant=release address-model=$bitlevel toolset=gcc runtime-link=static
./b2 -a  cxxflags='-fPIC -std=c++11' install --prefix=/usr/local link=static threading=multi boost.locale.icu=off