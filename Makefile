CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -O2 -fwrapv

.PHONY: all clean remake

all: mariolang

clean:
	rm mariolang

remake: clean
	$(MAKE) all
