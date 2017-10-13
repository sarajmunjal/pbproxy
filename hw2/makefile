CXX = g++
CXXFLAGS = -Wall -g
LDLAGS = -lpcap
mydump: ; $(CXX) $(CXXFLAGS) src/main.cpp src/sniffer.h -o ./bin/mydump $(LDLAGS)

clean: ; rm -rfv ./bin/mydump src/*.o src/*.gch