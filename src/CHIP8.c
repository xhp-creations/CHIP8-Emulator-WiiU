#include "CHIP8.h"

char* avaiable_keys[16]={"A","B","X","Y","LEFT","RIGHT","UP","DOWN","ZL","ZR","L","R","MINUS","PLUS","LSTICK","RSTICK"}; //Avaiable WiiU keys
unsigned char kconf[16]={0x5,0x10,0x10,0x10,0x4,0x6,0x2,0x8,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10}; //Default key configuration

void CHIP8_initialize(unsigned char *newROM, unsigned char *keyConf) {
	pc     = 0x200;  // Program counter starts at 0x200
	opcode = 0;      // Reset current opcode	
	I      = 0;      // Reset index register
	sp     = 0;      // Reset stack pointer
    mode   = 0;      // Reset mode
	memset(V,0,16); //Clear display
	memset(hp48_flags,0,8); //Clear display
	memset(gfx,0,8192); //Clear display
	memset(cstack,0,16); //Clear stack
    memcpy(&kconf, keyConf, 16);
	memset(key,0,17); //Reset keys
	memset(memory,0,4096); //Clear memory
	memcpy(&memory[512],newROM,3584*sizeof(char)); //Load rom into memory
	memcpy(memory,chip8_fontset,80*sizeof(char)); //Load fontset
	memcpy(&memory[80],bigfont,160*sizeof(char)); //Load fontset
	//Reset timers
	delay_timer = 0;
	sound_timer = 0;
}
