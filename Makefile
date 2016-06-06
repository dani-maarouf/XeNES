SRC_FILES    = src/main.cpp src/NES.cpp src/gameLoop.cpp
HEADER_FILES = include/NES.hpp include/gameLoop.hpp

CXXFLAGS     = -Wall -Iinclude

all: nesEmu

nesEmu: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) $(CXXFLAGS) -o nes -lSDL2


