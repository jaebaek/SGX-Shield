#!/bin/bash

mkdir -p tmp
objdump -d program > tmp/dump
g++ gen_symtab.cpp -o gen_symtab
./gen_symtab > tmp/symtab
./sort_ptrace.py tmp

rm gen_symtab tmp/dump tmp/symtab
