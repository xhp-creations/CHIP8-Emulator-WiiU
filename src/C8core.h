#ifndef CHIP8CORE_H
#define CHIP8CORE_H

#include <stdlib.h>
#include "main.h"
#include "CHIP8.h"

#ifdef __cplusplus
extern "C" {
#endif

void drawSprite(unsigned char X, unsigned char Y, unsigned char N);
void CHIP8_emulateCycle();
void CHIP8_decreaseTimers();
void CHIP8_setKeys(unsigned int buttonValue);

#ifdef __cplusplus
}
#endif

int crand();
int rand();


#endif /* CHIP8CORE_H */
