INSTALLDIR=/usr/local/bin

src = $(wildcard *.cpp *.c *.cxx)
csrc = $(wildcard *.c)
obj_cpp = $(src:.cpp=.o)
obj = $(obj_cpp:.c=.o)
hdr = $(wildcard *.h *.hpp)

CC=gcc
COPT=-g -lm
COPT2=-g -lm -DGERBER_TEST

#COPT=-O3 -lm
#COPT2=-O3 -lm -DGERBER_TEST

CXX=g++
CXXOPT=-g --std=c++11
#CXXOPT=-O3 --std=c++11
CXXLIB=-lstdc++ -lboost_thread -lm

CCFN = $(wildcard *.c)
CPPFN = $(wildcard *.cpp)

%.o: %.c %.h
	$(CC) $(COPT) -c $< -o $@ -lm

%.o: %.c
	$(CC) $(COPT) -c $< -o $@ -lm

%.o: %.cpp %.hpp
	$(CXX) $(CXXOPT) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXOPT) -c $< -o $@

%.o: %.cxx %.hxx
	$(CC) $(COPT) -c $< -o $@ -lm

gbr2ngc: $(obj) $(hdr)
	$(CXX) $(CXXOPT) -o $@ $(obj) $(CXXLIBS)

test_gerber_interpreter: $(csrc)
	$(CC) $(COPT2) -o $@ $^ $(COPT2)

.PHONY: debug
debug: $(obj) $(hdr)
	$(CXX) -g -o gbr2ngc_debug $^ $(CXXLIBS)

.PHONY: clean
clean:
	rm -f *.o gbr2ngc
	rm -f *.o test_gerber_interpreter

.PHONY: install
install: gbr2ngc
	mkdir -p $(INSTALLDIR)
	cp $< $(INSTALLDIR)
