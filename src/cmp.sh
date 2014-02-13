#!/bin/bash

#CPP_FILES="gbl2ngc.cpp gbl2ngc_debug.cpp gbl2ngc_export.cpp gbl2ngc_construct.cpp gbl2ngc_offset.cpp gbl2ngc_aperture.cpp gbl2ngc_globals.cpp"
CPP_FILES="gbl2ngc.cpp gbl2ngc_aperture.cpp gbl2ngc_globals.cpp gbl2ngc_construct.cpp gbl2ngc_export.cpp "

gcc -g gerber_interpreter.c -c 
gcc -g test_gerber_interpreter.c gerber_interpreter.o -o test_gerber_interpreter

#g++ -O2 -o gbl2ngc $CPP_FILES  gerber_interpreter.o -I./cpp -L. -L./cpp -lpolyclipping -lstdc++  -lboost_thread
g++ -O2 -o gbl2ngc $CPP_FILES  gerber_interpreter.o clipper.cpp -lstdc++  -lboost_thread

