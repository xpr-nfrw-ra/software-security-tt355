CXX = g++
CXXFLAGS = -std=c++17

ALL = packer.exe stub.exe crackme.exe solver.exe

all: $(ALL)

packer.exe: packer.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

stub.exe: stub.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

crackme.exe: crackme.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

solver.exe: solver.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(ALL) packed.bin unpacked.exe program.exe

.PHONY: all clean
