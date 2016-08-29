# Makefile for nbench, December 11, 1997, Uwe F. Mayer <mayer@tux.org>
# Updated February 18, 2003

default: nbench

##########################################################################
#   If you are using gcc-2.7.2.3 or earlier:
#   The optimizer of gcc has a bug and in general you should not specify
#   -funroll-loops together with -O (or -O2, -O3, etc.)
#   This bug is supposed to be fixed with release 2.8 of gcc.
#
#   This bug does NOT seem to have an effect on the correct compilation
#   of this benchmark suite on my Linux box. However, it leads to
#   the dreaded "internal compiler error" message on our alpha
#   running DEC Unix 4.0b. The Linux-binary that was used to obtain
#   the baseline results was nevertheless compiled with
#   CFLAGS = -s -static -Wall -O3 -fomit-frame-pointer -funroll-loops
#
# You should leave -static in the CFLAGS so that your sysinfo can be
# compiled into the executable.

CC = gcc

# generic options for gcc
CFLAGS = -s -static -Wall -O3

# if your gcc lets you do it, then try this one
#CFLAGS = -s -static -Wall -O3 -fomit-frame-pointer -funroll-loops

# for gcc on an older Pentium type processor you can try the following
#CFLAGS = -s -static -O3 -fomit-frame-pointer -Wall -m486 \
#	-fforce-addr -fforce-mem -falign-loops=2 -falign-functions=2 \
#	-falign-jumps=2 -funroll-loops

# for a newer gcc on a newer Pentium type processor you can try the following
#CFLAGS = -s -static -O3 -fomit-frame-pointer -Wall -march=i686 \
#	-fforce-addr -fforce-mem -falign-loops=2 -falign-functions=2 \
#	-falign-jumps=2 -funroll-loops

# for a newer gcc on an Athlon XP type processor you can try the following
#CFLAGS = -s -static -O3 -fomit-frame-pointer -Wall -march=athlon-xp \
#	-fforce-addr -fforce-mem -falign-loops=2 -falign-functions=2 \
#	-falign-jumps=2 -funroll-loops

# For debugging using gcc
#CFLAGS = -g -O3 -Wall -DDEBUG

##########################################################################
# For Linux machines with more than one binary format.
# The default binaries, depends on your system whether it's elf or aout.
MACHINE=
# a.out code for linux on an elf machine
#MACHINE= -bi486-linuxaout
# elf code for linux on an a.out machine
#MACHINE= -bi486-linuxelf
# if you want a different compiler version and different binaries, for example
#MACHINE= -V2.7.2 -bi486-linuxaout

##########################################################################
# Read the file README.nonlinux if you are not using Linux

# for DEC Unix using cc you can try
#CC = cc
#CFLAGS = -O3
#LINKFLAGS = -s -non_shared

# for SunOS using cc
#CC = cc
#CFLAGS = -O3 -s

# for DEC Ultrix using cc
#CC = cc
#CFLAGS = -O2
#LINKFLAGS = -s

# for a Mac with OsX and the Darwin environment
#CC = cc
#CFLAGS = -O3 -DOSX

# For debugging using cc
#CC = cc
#CFLAGS = -g -DDEBUG

##########################################################################
# If your system does not understand the system command "uname -s -r"
# then comment this out

# NO_UNAME= -DNO_UNAME

##########################################################################
# For any Unix flavor you need -DLINUX
# You also need -DLINUX to get the new indices

DEFINES= -DLINUX $(NO_UNAME)

nbench.a: emfloat.o misc.o nbench0.o nbench1.o sysspec.o
	$(AR) -rs $@ $^

nbench0.o: nbench0.h nbench0.c ../enclave.h nmglobal.h pointer.h Makefile
	$(CC) $(LINKERFLAG) $(MACHINE) $(DEFINES) $(CFLAGS)\
		-c nbench0.c

emfloat.o: emfloat.h emfloat.c nmglobal.h pointer.h Makefile
	$(CC) $(MACHINE) $(DEFINES) $(CFLAGS)\
		-c emfloat.c

pointer.h: Makefile
	echo "#define LONG64" >pointer.h

misc.o: misc.h misc.c Makefile
	$(CC) $(MACHINE) $(DEFINES) $(CFLAGS)\
		-c misc.c

nbench1.o: nbench1.h nbench1.c data.inc wordcat.h nmglobal.h pointer.h Makefile
	$(CC) $(MACHINE) $(DEFINES) $(CFLAGS)\
		-c nbench1.c

sysspec.o: sysspec.h sysspec.c nmglobal.h pointer.h Makefile
	$(CC) $(MACHINE) $(DEFINES) $(CFLAGS) -DNBENCH_TEST=1\
		-c sysspec.c

malloc.o: ../malloc/malloc.c ../malloc/dlmalloc.inc
	$(CC) $(MACHINE) $(DEFINES) $(CFLAGS) -DNBENCH_TEST=1\
		-c ../malloc/malloc.c -o malloc.o

nbench: emfloat.o misc.o nbench0.o nbench1.o sysspec.o malloc.o
	$(CC) $(MACHINE) $(DEFINES) $(CFLAGS) $(LINKFLAGS)\
		emfloat.o misc.o nbench0.o nbench1.o sysspec.o malloc.o\
		-o nbench -lm

##########################################################################

clean:
	- /bin/rm -f *.o *~ \#* core a.out hello sysinfo.c sysinfoc.c \
		 bug pointer pointer.h debugbit.dat

mrproper: clean
	- /bin/rm -f nbench
