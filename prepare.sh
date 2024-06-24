#!/bin/bash
set -xe

cd libs/libDaisy 
git submodule init 
git submodule update --recursive
make -j
cd ../../
