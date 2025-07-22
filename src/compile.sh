#!/bin/bash
# quick testing script
gcc -fuse-ld=gold main.c -o orpheus -I/usr/include -lmpdclient -lncurses -llua -g

./orpheus

