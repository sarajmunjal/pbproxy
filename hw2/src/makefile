CXX = g++
CXXFLAGS = -Wall -g
LDLAGS = -lpcap
mydump: ; $(CXX) $(CXXFLAGS) main.cpp sniffer.h -o mydump $(LDLAGS)

clean: ; rm -rfv mydump *.o *.gch