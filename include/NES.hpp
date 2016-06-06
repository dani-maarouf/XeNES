#ifndef __NES_NES__
#define __NES_NES__

#include <cstdint>

enum AddressMode {
    ABS, ABSX, ABSY, ACC, IMM, IMP, IND, INDX, INDY, REL, ZRP, ZRPX, ZRPY, NONE,
};

//is this a "god class"? need to think of how i can refeactor in sane way
class NES {

private:
    uint16_t PC;        //program counter
    uint8_t SP;         //stack pointer
    uint8_t A;          //accumulator
    uint8_t X;          //register X
    uint8_t Y;          //register Y
    bool PS[8];         //processor status word

    uint8_t ioRegisters[0x20];       
    uint8_t ppuRegisters[0x8];  

    int numRomBanks, numRamBanks;
    uint8_t * PRG_ROM, * PRG_RAM, * CHR_ROM;

    uint8_t cpuRAM[0x800];
    uint8_t cpuMem[0x10000];    //fallback memory addresses

    //get and set memory and addresses
    uint8_t getByte(uint16_t);
    bool setByte(uint16_t, uint8_t);
    uint16_t retrieveAddress(enum AddressMode, bool *);

public:

	int count;

	//initialize
    NES();
	bool openROM(const char *);

	//run
    int executeNextOpcode(bool);

    //deinitialize
    void freePointers();

};

#endif
/* DEFINED __NES_NES__ */
