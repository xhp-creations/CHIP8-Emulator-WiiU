#ifndef CHIP8_H
#define CHIP8_H

#include "main.h"
#include "C8core.h"

#include <errno.h>
#include <malloc.h>
#include <gctypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>

extern unsigned short opcode; //Current opcode
extern unsigned char memory[4096]; 
extern unsigned char V[16];
extern unsigned char hp48_flags[8];
extern unsigned short I;
extern unsigned short pc;
extern unsigned char delay_timer;
extern unsigned char sound_timer;
extern unsigned short cstack[16];
extern unsigned short sp;
extern unsigned char key[0x11];
extern unsigned char chip8_fontset[80];
extern unsigned char bigfont[160];
extern int li;
extern unsigned char kconf[16];
extern int mode;

#ifdef __cplusplus
extern "C" {
#endif

void CHIP8_initialize(unsigned char *newROM, unsigned char *keyConf);

#ifdef __cplusplus
}
#endif

#endif /* CHIP8_H */
