CXX=g++
LD=g++
CXXFLAGS=-m32 -DDEBUG
LDFLAGS=-lpe-parser-library -ldl -m32
DEPS=fontsub.h
SRCS=loader.cc
OBJS=$(subst .cc,.o,$(SRCS))

all: loader

%.o: %.cc $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

loader: $(OBJS)
	$(LD) -o $@ $< $(LDFLAGS)

clean:
	$(RM) loader $(OBJS)
