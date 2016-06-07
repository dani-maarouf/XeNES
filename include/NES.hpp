#ifndef __NES_NES__
#define __NES_NES__

#include <cstdint>

enum ProcessorStatusIndex {
    C = 0,       //carry flag
    Z = 1,      //zero flag
    I = 2,      //disable interrupts
    D = 3,      //BCD mode
    V = 6,      //overflow
    N = 7,      //is number negative
};

enum AddressMode {
    ABS, ABSX, ABSY, ACC, IMM, IMP, IND, INDX, INDY, REL, ZRP, ZRPX, ZRPY, NONE,
};

enum Mirroring {
	VERTICAL, HORIZONTAL, FOUR_SCREEN,
};

//is this a "god object"? need to think of how i can refeactor in sane way
class NES {

private:

	/* System */
	uint8_t ioRegisters[0x20];       


	/* CPU */ 
    uint16_t PC;        //program counter
    uint8_t SP;         //stack pointer
    uint8_t A;          //accumulator
    uint8_t X;          //register X
    uint8_t Y;          //register Y
    bool PS[8];         //processor status word
    uint8_t cpuRAM[0x800];		//2kB PPU RAM
    uint8_t * PRG_ROM;
    uint8_t * PRG_RAM;
    int numRomBanks;
    int numRamBanks;
    uint8_t cpuMem[0x10000];    //fallback memory addresses
    int cpuCycle;

    uint8_t getCpuByte(uint16_t);
    bool setCpuByte(uint16_t, uint8_t);
    uint16_t retrieveCpuAddress(enum AddressMode, bool *);


    /* PPU */

    uint8_t palette[0x20];

    uint8_t ppuRegisters[0x8];	//PPU registers
    uint8_t ppuRAM[0x800];		//2kB PPU RAM
    uint8_t ppuOAM[0x100];		//256 byte PPU OAM
    uint8_t * CHR_ROM;			//cartridge video ROM
    enum Mirroring mirroring;	//nametable arrangement
    int ppuCycle;    			//0-341
    bool usesRAM;				//true if CHR_RAM is used rather than CHR_ROM


	
public:

	/* System */
	//initialize
    NES();
	bool openCartridge(const char *);
	//deinitialize
    void freeCartridgePointers();


    /* CPU */
	//run, returns number of CPU cycles taken
    int executeNextOpcode(bool);
    int getCpuCycle();
    void setCpuCycle(int);


    /* PPU */

    uint8_t getPpuByte(uint16_t);
    bool setPpuByte(uint16_t, uint8_t);

    void ppuTick();
    int getPpuCycle();
    void setPpuCycle(int);
};

#endif
/* DEFINED __NES_NES__ */
