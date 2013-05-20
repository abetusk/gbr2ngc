#!/bin/bash

gcc -O3 gerber_interpreter.c -c 
gcc -O3 test_gerber_interpreter.c gerber_interpreter.o -o test_gerber_interpreter
g++ -O3 -frounding-math gerber_interpreter.o gbl2ngc.cpp -lstdc++ -lgmp -lCGAL -lCGAL_Core -lmpfr -lboost_thread -o gbl2ngc

#gcc gerber_interpreter.c test_gerber_interpreter.c -o test_gerber_interpreter
