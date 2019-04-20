INSTALLDIR=/usr/local/bin

src = $(wildcard *.cpp *.c)
csrc = $(wildcard *.c)
obj_cpp = $(src:.cpp=.o)
obj = $(obj_cpp:.c=.o)
hdr = $(wildcard *.h *.hpp)

CC=gcc
COPT=-g -lm
COPT2=-g -lm -DGERBER_TEST

CXX=g++
CXXOPT = -g --std=c++11
CXXLIB = -lstdc++ -lboost_thread -lm

CCFN = $(wildcard *.c)
CPPFN = $(wildcard *.cpp)

%.o: %.c
	$(CC) $(COPT) -c $< -o $@ -lm

%.o: %.cpp
	$(CXX) $(CXXOPT) -c $< -o $@
#	$(CXX) $(CXXOPT) -c $< -o $@ $(CXXLIB)

gbl2ngc: $(obj) $(hdr)
	$(CXX) $(CXXOPT) -o $@ $^ $(CXXLIBS)

test_gerber_interpreter: $(csrc)
	$(CC) $(COPT2) -o $@ $^ $(COPT2)

.PHONY: debug
debug: $(obj) $(hdr)
	$(CXX) -g -o gbl2ngc_debug $^ $(CXXLIBS)

.PHONY: clean
clean:
	rm -f *.o gbl2ngc
	rm -f *.o test_gerber_interpreter

.PHONY: install
install: gbl2ngc
	mkdir -p $(INSTALLDIR)
	cp $< $(INSTALLDIR)
