all: nes

nes: src/main.cpp src/CPU.cpp include/CPU.hpp src/PPU.cpp include/PPU.hpp src/NES.cpp include/NES.hpp
	g++ src/main.cpp src/CPU.cpp src/PPU.cpp src/NES.cpp -Iinclude -o nes
