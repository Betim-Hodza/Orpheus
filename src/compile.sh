#!/bin/bash
# quick testing script
gcc src/main.c -o orpheus -I/usr/include -lmpdclient -lncurses -llua5.3 -g

./orpheus
