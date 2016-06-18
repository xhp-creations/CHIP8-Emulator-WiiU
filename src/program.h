#ifndef PROGRAM_H
#define PROGRAM_H

#include <string.h>
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

extern bool CHIP8_drawFlag;
int _entryPoint();
void flipBuffers();


#endif /* PROGRAM_H */
