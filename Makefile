CXX?=g++
CXXFLAGS?=-O2 -std=c++17

all: reconstruction

reconstruction: reconstruction.cpp
	$(CXX) $(CXXFLAGS) -o reconstruction reconstruction.cpp

clean:
	rm -f reconstruction
