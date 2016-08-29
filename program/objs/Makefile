CURDIR = .

all: libc-aslr.a

libc-aslr.a: $(CURDIR)/libc.a
	$(AR) -x $(CURDIR)/libc.a
	$(AR) -rs libc-aslr.a *.o ../in_enclave_test.o

clean:
	rm -f *.o *.a
