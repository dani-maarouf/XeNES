SRC_FILES    = src/main.cpp src/NES.cpp src/gameLoop.cpp src/CPU.cpp src/PPU.cpp
HEADER_FILES = include/NES.hpp include/gameLoop.hpp

CXXFLAGS     = -Wall -Iinclude -pedantic -Wformat
LINK_FLAGS   = -lSDL2

all: nesEmu

nesEmu: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) $(CXXFLAGS) -o nes $(LINK_FLAGS)


