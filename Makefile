all: nes

nes: src/main.cpp src/CPU.cpp include/CPU.hpp
	g++ src/main.cpp src/CPU.cpp -Iinclude -o nes
