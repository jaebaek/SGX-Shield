# SGX-Shield


Introduction
------------
Hello! SGX-Shield is a system supporting ASLR in the SGX environment.
Currently, we submitted a paper based on this project in NDSS 2017.
The paper will be attached (if successfully published).

All implementations for this project (except the existing code base like LLVM)
is done by Jaebaek Seo (jaebaek at kaist dot ac dot kr).


Build and run
------------
###Install Intel SGX SDK for Linux:
- See `(rootdir)/linux-driver/README.md` and `(rootdir)/linux-sdk/README.md`


###Build LLVM
~~~~~{.sh}
$ cd (rootdir)/llvm
$ mkdir build
$ cmake -G 'Unix Makefiles' ../ -DCMAKE_BUILD_TYPE=Debug -DLLVM_ENABLE_ASSERTIONS=On -DLLVM_TARGETS_TO_BUILD="X86"
$ make  # or make -jN
~~~~~


###Build libraries and link program with them
- Generate ocall stubs.
~~~~~{.sh}
$ cd (rootdir)/program/lib/
$ ./gen_ocall_stub.py   # this python script works with Python 2.7.6
~~~~~

- Build libraries (musl-libc, libgcc, ocall stubs) and link program with them
~~~~~{.sh}
$ cd (rootdir)/program/
$ make CC="`pwd`/../llvm/build/bin/clang -fPIC -fno-asynchronous-unwind-tables -fno-jump-tables"
~~~~~


###Run in an enclave
~~~~~{.sh}
$ cp (rootdir)/program/program (rootdir)/loader
$ cd (rootdir)/loader/
$ make SGX_MODE=HW SGX_DEBUG=1
~~~~~


How to write another program
------------
- The entry function is `void enclave_main()` (See `(rootdir)/program/in_enclave_test.c`).
- It must be ended by calling `enclave_exit();`


Limitation
------------
- Some libc functions will not work because of system calls.
Currently, ocall stubs support system calls (See `(rootdir)/program/lib/`).


TODO
------------
###Documentation
- How to build nbenchmark.
- How to build mbedTLS and a ssl sample server.
- Debugging the program with non-enclave mode.
- How to add ocalls.
- ptrace/rop_attack mode.
- Add Windows version.

###Optimization
- Improve software-DEP loop optimization.
