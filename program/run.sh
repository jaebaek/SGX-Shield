#!/bin/bash

IN_ENC_LOADER=ld

if [[ "$1" == "-b" ]]; then
    shift
    make clean
    make CC="$1 -mcmodel=large -fno-asynchronous-unwind-tables"
    shift
fi

cp target.rel ../../
cd ../../
./opensgx -s libsgx/ld/${IN_ENC_LOADER}.sgx --key sign.key

case $1 in
    -d)
        ./opensgx -d $2 libsgx/ld/${IN_ENC_LOADER}.sgx libsgx/ld/${IN_ENC_LOADER}.conf
        ;;
    *)
        ./opensgx libsgx/ld/${IN_ENC_LOADER}.sgx libsgx/ld/${IN_ENC_LOADER}.conf
        ;;
esac

rm target.rel
