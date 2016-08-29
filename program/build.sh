#!/bin/bash

SGX_PATH=`pwd`"/../.."
CLANG=`pwd`"/../../llvm.aslr/build/bin/clang"
OPTION="-fPIC -fno-asynchronous-unwind-tables"

if [[ $# -ge 1 && $1 == "clean" ]]; then
    cd $SGX_PATH"/libsgx/"
    make clean
    cd $SGX_PATH"/libsgx/musl-libc"
    make clean
    cd $SGX_PATH"/libsgx/ld"
    make clean
    shift
fi

cd $SGX_PATH"/libsgx/"
echo ""
echo ""
echo "build target (libc-sgx.a, libsgx.a) in" `pwd`
make CC="$CLANG $OPTION" target

if [[ $# -ge 1 && $1 == "nben" ]]; then
    cd $SGX_PATH"/libsgx/ld"
    echo ""
    echo ""
    echo "build nbench for Windows SGX in" `pwd`
    make clean
    make CC="$CLANG $OPTION" nben
else
    cd $SGX_PATH"/libsgx/ld"
    echo ""
    echo ""
    echo "build wtarget.rel for Windows SGX in" `pwd`
    make clean
    make CC="$CLANG $OPTION" wtarget.rel
fi
