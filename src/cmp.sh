#!/bin/bash

#CPP_FILES="gbl2ngc.cpp gbl2ngc_debug.cpp gbl2ngc_export.cpp gbl2ngc_construct.cpp gbl2ngc_offset.cpp gbl2ngc_aperture.cpp gbl2ngc_globals.cpp"
CPP_FILES="gbl2ngc.cpp gbl2ngc_aperture.cpp gbl2ngc_globals.cpp gbl2ngc_construct.cpp gbl2ngc_export.cpp "
OBJ_FILES="gerber_interpreter.o tesexpr.o string_ll.o"

gcc -g gerber_interpreter.c gerber_interpreter_aperture_macro.c tesexpr.c string_ll.c -c  -lm
#gcc -g test_gerber_interpreter.c $OBJ_FILES  -o test_gerber_interpreter -lm


#gcc -O3 gerber_interpreter.c tesexpr.c string_ll.c -c  -lm
#gcc -O3 test_gerber_interpreter.c $OBJ_FILES  -o test_gerber_interpreter -lm

#g++ -O2 -o gbl2ngc $CPP_FILES  gerber_interpreter.o -I./cpp -L. -L./cpp -lpolyclipping -lstdc++  -lboost_thread
#g++ -Wall -O2 -o gbl2ngc $CPP_FILES  $OBJ_FILES clipper.cpp -lstdc++  -lboost_thread -lm
#g++ -std=c++11 -Wall -O2 -o gbl2ngc $CPP_FILES  $OBJ_FILES clipper.cpp -lstdc++  -lboost_thread -lm
g++ -std=c++11 -g -o gbl2ngc $CPP_FILES  $OBJ_FILES clipper.cpp -lstdc++  -lboost_thread -lm
#g++ -g -o gbl2ngc_debug $CPP_FILES  $OBJ_FILES clipper.cpp -lstdc++  -lboost_thread

