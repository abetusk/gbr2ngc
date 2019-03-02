INSTALLDIR=/usr/local/bin

src = $(wildcard *.cpp *.c)
obj_cpp = $(src:.cpp=.o)
obj = $(obj_cpp:.c=.o)
hdr = $(wildcard *.h *.hpp)

CC=gcc

CXX=g++
CXXOPT = --std=c++11
CXXLIB = -lstdc++ -lboost_thread -lm

CCFN = $(wildcard *.c)
CPPFN = $(wildcard *.cpp)

%.o: %.c
	$(CC) -c $< -o $@ -lm

%.o: %.cpp
	$(CXX) $(CXXOPT) -c $< -o $@
#	$(CXX) $(CXXOPT) -c $< -o $@ $(CXXLIB)

gbl2ngc: $(obj) $(hdr)
	$(CXX) -o $@ $^ $(CXXLIBS)

.PHONY: debug
debug: $(obj) $(hdr)
	$(CXX) -g -o gbl2ngc_debug $^ $(CXXLIBS)

.PHONY: clean
clean:
	rm -f *.o gbl2ngc

.PHONY: install
install: gbl2ngc
	mkdir -p $(INSTALLDIR)
	cp $< $(INSTALLDIR)
