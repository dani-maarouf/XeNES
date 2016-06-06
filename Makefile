SRC_FILES    = src/main.cpp src/CPU.cpp src/NES.cpp src/APU.cpp src/PPU.cpp src/gameLoop.cpp
HEADER_FILES = include/CPU.hpp include/PPU.hpp include/NES.hpp include/APU.hpp include/APU.hpp

CXXFLAGS     = -Wall -Iinclude

all: nesEmu

nesEmu: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) $(CXXFLAGS) -o nes -lSDL2


