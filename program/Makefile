BASE_CFLAGS = -Wall -pedantic -Wno-unused-function -std=gnu11

# SGX LIB
LIBCDIR = musl-libc
SGX_CFLAGS = $(BASE_CFLAGS) -I$(LIBCDIR)/include \
             -fno-stack-protector -fvisibility=hidden

LIBC = $(LIBCDIR)/lib/libc.a

STATIC_NOLIBC_FLAG = -nostdlib -nostartfiles

OCALL_OBJS_DIR = lib/
OCALL_LIB_MK = lib/ocall.mk

#$(OCALL_LIB_MK):
#	$(shell cd $(OCALL_OBJS_DIR); ./gen_ocall_stub.py)
include $(OCALL_LIB_MK)

ASM_OBJS = lib/stack.o lib/start.o lib/rop_gadget.o \
		   lib/code_inject.o lib/ocall.o lib/rand.o

LIBGCC = libgcc/ashldi3.o libgcc/ashrdi3.o libgcc/__divide_error.o 	\
		 libgcc/__divdi3.o libgcc/__divti3.o libgcc/__lshrdi3.o 	\
		 libgcc/multi3.o libgcc/__negti2.o libgcc/__udivdi3.o 		\
		 libgcc/__udivmoddi4.o libgcc/__udivmodti4.o 				\
		 libgcc/__udivti3.o libgcc/__umoddi3.o libgcc/__umodti3.o

all: $(LIBC) program

$(LIBC):
	(cd musl-libc; ./configure)
	make -C musl-libc lib/libc.a

https: program $(LIBSSL)
	cp mbedtls/*.o objs
	echo '' > objs/in_enclave_test.o
	rm objs/in_enclave_test.o
	ls objs/*.o > list
	cat list | ./tools/linker https

LIBSSL = mbedtls/libmbedtls.a
$(LIBSSL):
	$(MAKE) -C mbedtls

LIBNBENCH = nbench/nbench.a
nben: program $(LIBNBENCH)
	cp nbench/*.o objs
	rm objs/in_enclave_test.o
	ls objs/*.o > list
	cat list | ./tools/linker nben

$(LIBNBENCH):
	$(MAKE) -C nbench nbench.a

program: objs/libc-aslr.a tools/linker $(OCALL_OBJS) $(ASM_OBJS) $(LIBGCC)
	cp lib/*.o objs
	cp libgcc/*.o objs
	ls objs/*.o > list
	cat list | ./tools/linker program

lib/%.o: lib/%.s
	$(CC) -c -o $@ $<

libgcc/%.o: libgcc/%.c
	$(CC) -c -o $@ $<

tools/linker: tools/linkage_editor.cpp
	$(CXX) -o $@ $(LINKERFLAG) $^

objs/libc-aslr.a: in_enclave_test.o malloc.o $(LIBC)
	cp $^ objs/
	$(MAKE) -C objs
	cp malloc.o objs/

malloc.o: malloc/malloc.c malloc/dlmalloc.inc
	$(CC) -c $(SGX_CFLAGS) -o $@ $<

BENCHMARK=$(wildcard benchmark/*.c)
in_enclave_test.o: in_enclave_test.c $(BENCHMARK) enclave.h
	$(CC) -c $(SGX_CFLAGS) -I. -o $@ $(LINKERFLAG) $<

clean:
	rm -f *.o *.a objs/*.o objs/*.a list program nben \
		tools/linker lib/*.o

clean-all:
	rm -f *.o *.a objs/*.o objs/*.a list program nben \
		tools/linker lib/*.o
	$(MAKE) -C nbench clean
	$(MAKE) -C mbedtls clean
	$(MAKE) -C musl-libc clean
