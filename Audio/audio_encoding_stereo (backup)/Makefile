# Path to your include files
INC=-I /home/ytan/Dev/build-x64/ffmpeg-x64/include
# Path to your include files
LIB=-L /home/ytan/Dev/build-x64/ffmpeg-x64/lib 
# lib files to be included
LIBRARIES= -lstdc++ -lavcodec -lavformat -lavutil
# Other C flags
CFLAGS=-std=c++0x -g


all: test #ptest

test: test.cpp 
	g++ $^ -o $@ $(INC) $(LIB) $(CFLAGS) $(LIBRARIES) 

clean:
	rm -f tmp/*.wav
	rm -f test

