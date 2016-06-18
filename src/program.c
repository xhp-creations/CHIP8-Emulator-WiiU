#include "program.h"

VPADData vpad_data; //store VPADData fromvpad library in structure vpad_data
int error; //error for vpad data

bool exitpg;

int buf0_size; //Global declaration for more speed when flipping buffers
int buf1_size;
void* buf0sff; //Used so in continuous flipbuffers we have one or two assembly instruction less (not so much but anyway why not to do so?)

int _entryPoint() {
	InitOSFunctionPointers();
	InitFSFunctionPointers();
	InitVPadFunctionPointers();

	mount_sd_fat("sd");

	VPADInit(); //Init VPAD
	OSScreenInit(); //Call the Screen initilzation function.
	buf0_size = OSScreenGetBufferSizeEx(0); // Grab the buffer size for
	buf1_size = OSScreenGetBufferSizeEx(1); // each screen (TV and gamepad)
	buf0sff = (void *)0xF4000000+buf0_size;
	//Set the buffer area.
	OSScreenSetBufferEx(0,(void *)0xF4000000);
	OSScreenSetBufferEx(1,buf0sff);
	for (int ii = 0; ii < 2; ii++) {
		OSScreenClearBufferEx(0, 0x00000000);
		OSScreenClearBufferEx(1, 0x00000000);
		flipBuffers();
	}
    	OSScreenEnableEx(0, 1);
    	OSScreenEnableEx(1, 1);
	
	MountVirtualDevices();

	LoadROM();
	if (exitpg) goto exit;
	
	int64_t (*OSGetTime)();
	OSDynLoad_FindExport(coreinit_handle, 0, "OSGetTime", &OSGetTime);   
	srand(OSGetTime()); //Random seed

	CHIP8_initialize(); //Init the emulator
	
	while(!exitpg) { //Emulation loop
		CHIP8_emulateCycle(); // Emulate one cycle
		if(CHIP8_drawFlag) CHIP8_drawGraphics(); // If the draw flag is set, update the screen
		CHIP8_setKeys(); // Store key press state (Press and Release)	
		usleep(dbc);
	}
	
exit:
	for(int ii = 0;ii<2;ii++) { //WARNING: DO NOT CHANGE THIS. YOU MUST CLEAR THE FRAMEBUFFERS AND IMMEDIATELY CALL EXIT FROM THIS FUNCTION. RETURNING TO LOADER CAUSES FREEZE.
		OSScreenClearBufferEx(0, 0x00000000);
		OSScreenClearBufferEx(1, 0x00000000);
		flipBuffers();
	}
	UnmountVirtualPaths();
	unmount_sd_fat("sd");
	return 0;
}

void flipBuffers() {
	DCFlushRange(buf0sff,buf1_size); //Flush the cache
	DCFlushRange((void*)0xF4000000,buf0_size);
	OSScreenFlipBuffersEx(0); //Flip the buffer
	OSScreenFlipBuffersEx(1);
}
