CXX_FLAGS=-Wall -D__CL_ENABLE_EXCEPTIONS -DCL_HPP_TARGET_OPENCL_VERSION=220

all: bz

bz: bz.cpp
	g++ $(CXX_FLAGS) bz.cpp -o bz -g -lglut -lGLEW -lGLU -lGL `pkg-config --libs --cflags OpenCL`

clean:
	rm -rf bz
