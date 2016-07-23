#ifndef MAIN_H
#define MAIN_H

#include <string.h>
#include <gd.h>
#include "common/types.h"
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include "utils/utils.h"
#include "common/common.h"
#include "fs/virtualpath.h"
#include "CHIP8.h"

extern unsigned char gfx[128][64];
extern u32 renderTexture[256 * 128];  // comment to use gd image for playfield and uncomment gdimage below
//extern gdImagePtr virtualDisp; // uncomment to use gd image for playfield and comment renderTexture above
extern bool CHIP8_drawFlag;
extern bool CHIP8_soundFlag;
extern bool emulateROM;
extern int controlUsed;

#ifdef __cplusplus
extern "C" {
#endif

int _entryPoint();

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
