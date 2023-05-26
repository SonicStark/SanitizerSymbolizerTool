#!/bin/bash

set -euo pipefail

DIR_CUR="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )" #https://stackoverflow.com/a/4774063
DIR_LIB=`dirname $DIR_CUR`/lib
DIR_INC=`dirname $DIR_CUR`/include

COMMON_FLAG="-g -O0 -fsanitize=address -fsanitize=undefined -I$DIR_INC"

$CC  $COMMON_FLAG -c $DIR_CUR/simple_demo.c           -o $DIR_CUR/demo-main-tmp.o
$CXX $COMMON_FLAG -c $DIR_LIB/interface.cpp           -o $DIR_CUR/demo-interface-tmp.o
$CXX $COMMON_FLAG -c $DIR_LIB/common.cpp              -o $DIR_CUR/demo-common-tmp.o
$CXX $COMMON_FLAG -c $DIR_LIB/symbolizer.cpp          -o $DIR_CUR/demo-symbolizer-tmp.o
$CXX $COMMON_FLAG -c $DIR_LIB/use_llvm_symbolizer.cpp -o $DIR_CUR/demo-llvm-tmp.o
$CXX $COMMON_FLAG -c $DIR_LIB/use_addr2line.cpp       -o $DIR_CUR/demo-ad2l-tmp.o

$CXX $COMMON_FLAG \
        $DIR_CUR/demo-main-tmp.o \
        $DIR_CUR/demo-interface-tmp.o \
        $DIR_CUR/demo-common-tmp.o \
        $DIR_CUR/demo-symbolizer-tmp.o \
        $DIR_CUR/demo-llvm-tmp.o \
        $DIR_CUR/demo-ad2l-tmp.o \
-o $DIR_CUR/simple_demo

rm -f $DIR_CUR/demo-*-tmp.o

echo "ALL DONE"