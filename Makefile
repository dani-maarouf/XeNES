SRC_FILES    = src/main.cpp src/NES.cpp src/gameLoop.cpp src/CPU.cpp src/PPU.cpp
HEADER_FILES = include/NES.hpp include/PPU.hpp include/CPU.hpp include/gameLoop.hpp

CXXFLAGS     = -Wall -Iinclude -pedantic -fno-exceptions -Ofast -std=c++14 -march=native -fomit-frame-pointer
LINK_FLAGS   = -lSDL2 -lstdc++


all: nesLinux

nesLinux: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) $(CXXFLAGS) -o XeNES $(LINK_FLAGS)

# requires SDL2.dll
nesWindows: $(SRC_FILES) $(HEADER_FILES)
	x86_64-w64-mingw32-g++ $(SRC_FILES) $(CXXFLAGS) -o XeNES.exe -lmingw32 -lSDL2main -lSDL2 -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic

nesMac: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) -Iinclude -I/usr/local/include -o XeNES -L/usr/local/lib -lSDL2 -lstdc++ -fno-exceptions -Ofast -std=c++14 -march=native -fomit-frame-pointer

profile: $(SRC_FILES) $(HEADER_FILES)
	g++ $(SRC_FILES) -Iinclude -o XeNES -lSDL2 -pg

