#!/bin/bash

nasm -f elf64 find_prime.asm -o find_prime.o
gcc -O0 main.c find_prime.o -o lily -lm
./lily