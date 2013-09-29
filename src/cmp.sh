#!/bin/bash

CPP_FILES="gbl2ngc.cpp gbl2ngc_debug.cpp gbl2ngc_export.cpp gbl2ngc_construct.cpp gbl2ngc_offset.cpp gbl2ngc_aperture.cpp gbl2ngc_globals.cpp"

gcc -O3 gerber_interpreter.c -c 
gcc -O3 test_gerber_interpreter.c gerber_interpreter.o -o test_gerber_interpreter
g++ -O3 -frounding-math gerber_interpreter.o $CPP_FILES -lstdc++ -lgmp -lCGAL -lCGAL_Core -lmpfr -lboost_thread -o gbl2ngc

#gcc -O2 gerber_interpreter.c -c 
#gcc -O2 test_gerber_interpreter.c gerber_interpreter.o -o test_gerber_interpreter
#g++ -O2 -frounding-math gerber_interpreter.o $CPP_FILES -lstdc++ -lgmp -lCGAL -lCGAL_Core -lmpfr -lboost_thread -o gbl2ngc

#gcc -g gerber_interpreter.c -c 
#gcc -g test_gerber_interpreter.c gerber_interpreter.o -o test_gerber_interpreter
#g++ -g -frounding-math gerber_interpreter.o $CPP_FILES -lstdc++ -lgmp -lCGAL -lCGAL_Core -lmpfr -lboost_thread -o gbl2ngc

#gcc gerber_interpreter.c -c 
#gcc test_gerber_interpreter.c gerber_interpreter.o -o test_gerber_interpreter
#g++ -frounding-math gerber_interpreter.o $CPP_FILES -lstdc++ -lgmp -lCGAL -lCGAL_Core -lmpfr -lboost_thread -o gbl2ngc

#gcc gerber_interpreter.c test_gerber_interpreter.c -o test_gerber_interpreter
