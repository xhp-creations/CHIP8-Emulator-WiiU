#include "C8gfx.h"

unsigned char gfx[64 * 32]; //Screen
bool CHIP8_drawFlag;

int sx,sy,rx,ry,cx,cy; //declare var globaly to avoid redeclaration each loop
void CHIP8_drawGraphics() { //Draw the CHIP8 fb to screen
	OSScreenClearBufferEx(0, 0x00000001);
	OSScreenClearBufferEx(1, 0x00000001);
	for(cy = 0; cy < 32; cy++) {
		for(cx = 0; cx < 64; cx++) {
			if(!(gfx[(cy*64) + cx] == 0)) {
				rx=(cx*13);
				ry=(cy*13);
				for(sx=0;sx<13;sx++) {
					for(sy=0;sy<13;sy++) {
						OSScreenPutPixelEx(0,(rx+sx),(ry+sy),0xffffff01);
						OSScreenPutPixelEx(1,(rx+sx),(ry+sy),0xffffff01);
					}
				}
			}		
		}
	}
	flipBuffers();
	CHIP8_drawFlag = false;
}

