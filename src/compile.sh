#!/bin/bash
gcc main.c -o cbmp -I/usr/include -lmpdclient -lncurses -llua -g

./cbmp
