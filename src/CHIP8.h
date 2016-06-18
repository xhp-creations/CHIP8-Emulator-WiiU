#ifndef CHIP8_H
#define CHIP8_H

#include "fs/vrt.h"
#include "fs/virtualpath.h"
#include "program.h"
#include "C8core.h"
#include "C8gfx.h"

#include <errno.h>
#include <malloc.h>
#include <gctypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>

extern unsigned short opcode; //Current opcode
extern unsigned char memory[4096]; 
extern unsigned char V[16];
extern unsigned short I;
extern unsigned short pc;
extern unsigned char delay_timer;
extern unsigned char sound_timer;
extern unsigned short cstack[16];
extern unsigned short sp;
extern unsigned char key[0x11];
extern unsigned char chip8_fontset[80];
extern int li;

void CHIP8_initialize();
void emu_settings();
void emu_settings_render();
void emu_settings_render_tv();
void emu_settings_kconf();
void emu_settings_kconf_render();
int set_kconf_key(char* key_to_change);
void set_kconf_key_render(char* key_to_change);
char* emu_settings_getkey(int hexval);

void LoadROM();
bool check_extension(char* str);
void rom_choose_render(char files[16][256], int fnum, int selected);
void rom_choose_render_tv(char files[16][256], int fnum, int selected);

#endif /* CHIP8_H */
