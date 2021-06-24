#!/bin/bash
set -e
mkdir -p build
cd code
make -f ./Makefile
cd ..
