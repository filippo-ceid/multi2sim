#!/bin/bash

# Need to install valgrind

EXE_CMD="./mcf_base.i386 inp.in"

valgrind --tool=massif --pages-as-heap=yes --massif-out-file=massif.out $EXE_CMD
echo "The Peak Memory usage: "
cat massif.out | grep mem_heap_B | sed -e 's/mem_heap_B=\(.*\)/\1/' | sort -g | tail -n 1

