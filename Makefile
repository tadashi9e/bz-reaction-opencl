CXXFLAGS=-g -Wall `pkg-config --cflags OpenCL glut glu gl`
LDFLAGS=`pkg-config --libs OpenCL glut glu gl`

all: bz

bz: bz.cpp
	g++ $(CXXFLAGS) bz.cpp -o bz $(LDFLAGS)

clean:
	rm -rf bz
