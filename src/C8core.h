#ifndef CHIP8CORE_H
#define CHIP8CORE_H

#include <stdlib.h>
#include "program.h"
#include "CHIP8.h"

extern unsigned char gfx[64 * 32];
extern int kconf[13];
extern VPADData vpad_data;
extern int error;
extern bool exitpg;

void CHIP8_emulateCycle();
void CHIP8_setKeys();
int crand();
int rand();


#endif /* CHIP8CORE_H */
