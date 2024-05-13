CXX=g++
CXXFLAGS=-std=c++17 -g -pedantic -fsanitize=address,undefined -fno-omit-frame-pointer 
LDLIBS=

SRCS=main.cpp
DEPS=
OBJS=$(DEPS:%.cpp=%.o)

EXEC=zipper

all: clean $(EXEC)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(EXEC): $(SRCS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: clean test

clean:
	rm -f $(EXEC)
