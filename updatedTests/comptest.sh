#!/bin/bash
g++ --std=c++11 -Wall -g -pedantic-errors  -Werror malloc_4.h test.cpp -o tester
./tester