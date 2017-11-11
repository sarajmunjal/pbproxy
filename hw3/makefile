CXX = gcc
CXXFLAGS = -Wall -g
LDLAGS = -pthread
pbproxy: ; $(CXX) $(CXXFLAGS) src/pbproxy.c -o ./bin/pbproxy $(LDLAGS)

clean: ; rm -rfv ./bin/mydump src/*.o src/*.gch