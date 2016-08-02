SRC_FILES    = src/main.cpp src/NES.cpp src/gameLoop.cpp src/CPU.cpp src/PPU.cpp src/mappers.cpp
HEADER_FILES = include/NES.hpp include/PPU.hpp include/CPU.hpp include/gameLoop.hpp include/mappers.hpp
CXXFLAGS     = -std=c++14 -Wall -Wextra -Wwrite-strings -Wshadow -Wstrict-overflow=4 -Wno-unused-parameter -pedantic -Iinclude -Ofast -fno-exceptions -funroll-loops -ffast-math -frename-registers -march=native #-Wconversion -Weffc++
WINFLAGS     = -lmingw32 -lSDL2main -lSDL2 -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic -lpthread
LINK_FLAGS   = -lSDL2 -lstdc++

all: linux

linux: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) $(CXXFLAGS) -o XeNES $(LINK_FLAGS)

# binary will require SDL2.dll to run
windows: $(SRC_FILES) $(HEADER_FILES)
	x86_64-w64-mingw32-g++ $(SRC_FILES) $(CXXFLAGS) -o XeNES.exe $(WINFLAGS)

mac: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) $(CXXFLAGS) -I/usr/local/include -o XeNES -L/usr/local/lib $(LINK_FLAGS)

profile: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) -Wall -Werror -Iinclude -pg -o XeNES $(LINK_FLAGS)  

fastProfile: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) -Wall -Werror -Iinclude -pg -Ofast -o XeNES $(LINK_FLAGS) 

debug: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) -Wall -Werror -Iinclude -g -o XeNES $(LINK_FLAGS)
	

