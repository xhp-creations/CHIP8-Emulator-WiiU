#include "init.h"
#include "loader.h"

extern char __rx_start, __rx_end;
extern char __rw_start, __rw_end;

static unsigned char crom[2048]; //Chip8 rom "buffer"

unsigned short opcode; //Current opcode
unsigned char memory[4096]; /* CHIP8 memory (4K):
			       0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
			       0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
			       0x200-0xFFF - Program ROM and work RAM */
unsigned char V[16]; //CHIP8 Registers (1 reg=8 bit=1 byte=char)
unsigned short I; //Index register
unsigned short pc; //CHIP8 Program counter
unsigned char gfx[64 * 32]; //Screen
unsigned char delay_timer; //CHIP8 Delay timer
unsigned char sound_timer; //CHIP8 Sound timer
unsigned short cstack[16]; //CHIP8 Stack
unsigned short sp;
unsigned char key[0x11]; //State of buttons; we assign 1 more=/
bool CHIP8_drawFlag;
unsigned char chip8_fontset[80] = { 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

//Some global declarations used to avoid redeclarations
bool keyPress = false; //Used in an opcode; declared here to avoid continuos redeclarations
int lg,xline,yline,lh,li,lj; //used in 'for' loops
unsigned short kpixel,kheight,kx,ky;

void _main()
{
	//CURL-functions
	OSDynLoad_Acquire("nn_ac.rpl", &nn_ac_handle);
	OSDynLoad_Acquire("nsysnet", &nsysnet_handle);
	OSDynLoad_Acquire("nlibcurl", &libcurl_handle);
	OSDynLoad_FindExport(nn_ac_handle, 0, "ACInitialize", &ACInitialize);
	OSDynLoad_FindExport(nn_ac_handle, 0, "ACGetStartupId", &ACGetStartupId);   
	OSDynLoad_FindExport(nn_ac_handle, 0, "ACConnectWithConfigId",&ACConnectWithConfigId);
	OSDynLoad_FindExport(nsysnet_handle, 0, "socket_lib_init", &socket_lib_init);
	OSDynLoad_FindExport(libcurl_handle, 0, "curl_global_init", &curl_global_init);
	OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_init", &curl_easy_init);
	OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_cleanup", &curl_easy_cleanup);
	OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_setopt", &curl_easy_setopt);
	OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_perform", &curl_easy_perform);
	OSDynLoad_FindExport(libcurl_handle, 0, "curl_easy_getinfo", &curl_easy_getinfo);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSAllocFromSystem", &OSAllocFromSystem);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSFreeToSystem", &OSFreeToSystem);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSCreateThread", &OSCreateThread);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSResumeThread", &OSResumeThread);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSIsThreadTerminated", &OSIsThreadTerminated);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSDynLoad_Release", &OSDynLoad_Release);
	//Prepare for allocating thread for curl but don't do it
	void *stack = OSAllocFromSystem(0x5000, 0x10); //Allocate a stack for the thread
	void *thread = OSAllocFromSystem(OSTHREAD_SIZE, 8); //Create the thread
	//Export other functions for the emulator and the keyboard
	OSDynLoad_Acquire("vpad.rpl", &vpadHandle);
	OSDynLoad_FindExport(vpadHandle, 0, "VPADRead", &VPADRead);
	OSDynLoad_FindExport(coreinit_handle, 0, "DCFlushRange", &DCFlushRange);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenFlipBuffersEx", &OSScreenFlipBuffersEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenPutPixelEx", &OSScreenPutPixelEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenPutFontEx", &OSScreenPutFontEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenClearBufferEx", &OSScreenClearBufferEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenGetBufferSizeEx", &OSScreenGetBufferSizeEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenSetBufferEx", &OSScreenSetBufferEx);
	OSDynLoad_FindExport(coreinit_handle, 0, "OSGetTime", &OSGetTime);   
	OSDynLoad_FindExport(coreinit_handle, 0, "memset", &memset);

	//Export memory allocation function
	unsigned int *functionPointer = 0;
	OSDynLoad_FindExport(coreinit_handle, 1, "MEMAllocFromDefaultHeap", &functionPointer);
	MEMAllocFromDefaultHeap = (void*(*)(unsigned int))*functionPointer;

	// Set up render system
	_osscreeninit();
	//Clear both buffers
	OSScreenClearBufferEx(0, 0x00000001);
	OSScreenClearBufferEx(1, 0x00000001);
	flipBuffers();
	OSScreenClearBufferEx(0, 0x00000001);
	OSScreenClearBufferEx(1, 0x00000001);
	flipBuffers();

	kb_input(); //Require the user keyboard input
	fname = (char*)MEMAllocFromDefaultHeap(sizeof (char) * kb_text_len); //Alloc the array fname by the size of the entered string
	int k; 
	for(k=0;k<kb_text_len;k++) //Copy the string in the 'exact size char array'
		fname[k] = kb_text[k];

	int ret = OSCreateThread(thread, _mainThread, 0, (void*) 0, (uint32_t)stack + 0x1800, 0x1800, 0, 0x1A); //Create the curl thread
	if (!ret) OSFatal("Failed to create thread");

	OSResumeThread(thread); //Schedule curl thread for execution
	while(!OSIsThreadTerminated(thread)) ; //Keep this thread alive to do actual emulation
	OSDynLoad_Release(libcurl_handle); //Release libcurl as not needed 

	CHIP8_initialize(); //Initialize the Chip8 system and load the game into the memory  
 
	for(;;) //Emulation loop
	{
		CHIP8_emulateCycle(); // Emulate one cycle
		if(CHIP8_drawFlag) CHIP8_drawGraphics(); // If the draw flag is set, update the screen
		CHIP8_setKeys(); // Store key press state (Press and Release)	
	}
	_osscreenexit(); //Not needed
}
//--Code for rom download from LiteNES--
void _mainThread(int argc, void *argv) {
	char* leaddr = (char*)0;
	char* str, url[64];
	for (str = (char*)0x1A000000; str < (char*)0x20000000; str++) {
		/* Search for /payload which we use to find the URL */
		if(*(uint32_t*)str == 0x2F706179 && *(uint32_t*)(str + 4) == 0x6C6F6164) {
			leaddr = str;
			while(*leaddr) leaddr--;
			leaddr++;
			if(*(uint32_t*)leaddr == 0x68747470) break; /* If string starts with http its likely to be correct */
			leaddr = (char*)0;
		}
	}
	if(leaddr == (char*)0) OSFatal("Unable to find usable URL");
	/* Generate the boot.elf address */
	memcopy(url, leaddr, 64);
	char *ptr = url;
	while(*ptr) ptr++;
	while(*ptr != 0x2F) ptr--;
	memcopy(ptr + 1, fname, (kb_text_len+1));

	ACInitialize();
	ACGetStartupId(&nn_startupid);
	ACConnectWithConfigId(nn_startupid);
	socket_lib_init();
	curl_global_init(CURL_GLOBAL_ALL); 

	void *curl_handle = curl_easy_init();
	if(!curl_handle) OSFatal("cURL not initialized");
	curl_download_file(curl_handle, url);
	curl_easy_cleanup(curl_handle);
}
unsigned int offset = 0x01;
static int curl_write_data_callback(void *buffer, int size, int nmemb, void *userp) {
	int insize = size * nmemb;
	memcopy(crom + offset, buffer, insize);
	offset += insize;
	return insize;
}
static int curl_download_file(void *curl, char *url) {
	curl_easy_setopt(curl, CURLOPT_URL, url); //Setup parameters for file
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_callback);
	int ret = curl_easy_perform(curl); //Actually download the file
	if (ret) OSFatal("curl_easy_perform returned an error");
	int resp = 404;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp);
	if(resp != 200) OSFatal("curl_easy_getinfo returned an HTTP error");
}
//--End of code for rom download--
void CHIP8_initialize() {
	pc     = 0x200;  // Program counter starts at 0x200
	opcode = 0;      // Reset current opcode	
	I      = 0;      // Reset index register
	sp     = 0;      // Reset stack pointer
	
	
	// Clear display
	memset(gfx,0,2048);
	// Clear stack
	memset(cstack,0,16);
	//Reset keys
	memset(key,0,17);
	// Clear memory
	memset(memory,0,4096);	
	//TODO: Switch to memcpy()
	//Load rom into memory
	for(li = 0; li < 2048; li++)
		memory[li + 512] = crom[li];
	// Load fontset
	for(li = 0; li < 80; li++)
		memory[li] = chip8_fontset[li];		
	// Reset timers
	delay_timer = 0;
	sound_timer = 0;
}
void CHIP8_emulateCycle() {
  // Fetch opcode
  opcode = memory[pc] << 8 | memory[pc + 1];
 
  // Decode opcode
  switch(opcode & 0xF000)
	{		
		case 0x0000:
			switch(opcode & 0x000F)
			{
				case 0x0000:
					memset(gfx,0,2048);
					CHIP8_drawFlag = true;
					pc += 2;
				break;

				case 0x000E:
					sp--;
					pc = cstack[sp];
					pc += 2;
				break;

				default:
					;//printf ("Unknown opcode [0x0000]: 0x%X\n", opcode);					
			}
		break;

		case 0x1000:
			pc = opcode & 0x0FFF;
		break;

		case 0x2000:
			cstack[sp] = pc;
			sp++;
			pc = opcode & 0x0FFF;
		break;
		
		case 0x3000:
			if(V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
				pc += 4;
			else
				pc += 2;
		break;
		
		case 0x4000:
			if(V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
				pc += 4;
			else
				pc += 2;
		break;
		
		case 0x5000:
			if(V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
				pc += 4;
			else
				pc += 2;
		break;
		
		case 0x6000:
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
			pc += 2;
		break;
		
		case 0x7000:
			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
			pc += 2;
		break;
		
		case 0x8000:
			switch(opcode & 0x000F)
			{
				case 0x0000:
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0001:
					V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0002:
					V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0003:
					V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0004:
					if(V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) 
						V[0xF] = 1;
					else 
						V[0xF] = 0;					
					V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
					pc += 2;					
				break;

				case 0x0005:
					if(V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) 
						V[0xF] = 0;
					else 
						V[0xF] = 1;					
					V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
					pc += 2;
				break;

				case 0x0006:
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
					V[(opcode & 0x0F00) >> 8] >>= 1;
					pc += 2;
				break;

				case 0x0007:
					if(V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
						V[0xF] = 0;
					else
						V[0xF] = 1;
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];				
					pc += 2;
				break;

				case 0x000E:
					V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
					V[(opcode & 0x0F00) >> 8] <<= 1;
					pc += 2;
				break;

				default:
					;//printf ("Unknown opcode [0x8000]: 0x%X\n", opcode);
			}
		break;
		
		case 0x9000:
			if(V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
				pc += 4;
			else
				pc += 2;
		break;

		case 0xA000:
			I = opcode & 0x0FFF;
			pc += 2;
		break;
		
		case 0xB000:
			pc = (opcode & 0x0FFF) + V[0];
		break;
		
		case 0xC000:
			V[(opcode & 0x0F00) >> 8] = (rand() % 0xFFF) & (opcode & 0x00FF);
			pc += 2;
		break;
	
		case 0xD000:
		{
			kx = V[(opcode & 0x0F00) >> 8];
			ky = V[(opcode & 0x00F0) >> 4];
			kheight = opcode & 0x000F;

			V[0xF] = 0;
			for (yline = 0; yline < kheight; yline++)
			{
				kpixel = memory[I + yline];
				for(xline = 0; xline < 8; xline++)
				{
					if((kpixel & (0x80 >> xline)) != 0)
					{
						if(gfx[(kx + xline + ((ky + yline) * 64))] == 1)
						{
							V[0xF] = 1;                                    
						}
						gfx[kx + xline + ((ky + yline) * 64)] ^= 1;
					}
				}
			}
						
			CHIP8_drawFlag = true;			
			pc += 2;
		}
		break;
			
		case 0xE000:
			switch(opcode & 0x00FF)
			{
				case 0x009E:
					if(key[V[(opcode & 0x0F00) >> 8]] != 0)
						pc += 4;
					else
						pc += 2;
				break;
				
				case 0x00A1:
					if(key[V[(opcode & 0x0F00) >> 8]] == 0)
						pc += 4;
					else
						pc += 2;
				break;

				default:
					;//printf ("Unknown opcode [0xE000]: 0x%X\n", opcode);
			}
		break;
		
		case 0xF000:
			switch(opcode & 0x00FF)
			{
				case 0x0007:
					V[(opcode & 0x0F00) >> 8] = delay_timer;
					pc += 2;
				break;
								
				case 0x000A:
				{
					for(lh = 0; lh < 16; lh++)
					{
						if(key[lh] != 0)
						{
							V[(opcode & 0x0F00) >> 8] = lh;
							keyPress = true;
						}
					}
					if(!keyPress)						
						return;

					pc += 2;					
				}
				break;
				
				case 0x0015:
					delay_timer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
				break;

				case 0x0018:
					sound_timer = V[(opcode & 0x0F00) >> 8];
					pc += 2;
				break;

				case 0x001E:
					if(I + V[(opcode & 0x0F00) >> 8] > 0xFFF)
						V[0xF] = 1;
					else
						V[0xF] = 0;
					I += V[(opcode & 0x0F00) >> 8];
					pc += 2;
				break;

				case 0x0029:
					I = V[(opcode & 0x0F00) >> 8] * 0x5;
					pc += 2;
				break;

				case 0x0033:
					memory[I]     = V[(opcode & 0x0F00) >> 8] / 100;
					memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;					
					pc += 2;
				break;

				case 0x0055:
					for (li = 0; li <= ((opcode & 0x0F00) >> 8); li++)
						memory[I + li] = V[li];
					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
				break;

				case 0x0065:
					for (lj = 0; lj <= ((opcode & 0x0F00) >> 8); lj++)
						V[lj] = memory[I + lj];
					I += ((opcode & 0x0F00) >> 8) + 1;
					pc += 2;
				break;

				default:
					;//printf ("Unknown opcode [0xF000]: 0x%X\n", opcode);
			}
		break;

		default:
			;//printf ("Unknown opcode: 0x%X\n", opcode);
	}
 
  // Update timers
  if(delay_timer > 0)
    delay_timer--;
 
  if(sound_timer > 0)
  {
    if(sound_timer == 1) 
    	//No sound for now :P
    sound_timer--;
  }  
}
void CHIP8_setKeys(){
	VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
	if (vpad_data.btn_hold & BUTTON_HOME)	ExitApplication();
	if (vpad_data.btn_hold & BUTTON_PLUS) {	
		emu_settings();
		unsigned int dbe = 0x2000000;
		while(dbe--) ; 
	}

	if(vpad_data.btn_hold & BUTTON_A) 	{ key[kconf[0]] = 1; } else { key[kconf[0]] = 0; }
	if(vpad_data.btn_hold & BUTTON_B) 	{ key[kconf[1]] = 1; } else { key[kconf[1]] = 0; }
	if(vpad_data.btn_hold & BUTTON_X) 	{ key[kconf[2]] = 1; } else { key[kconf[2]] = 0; }
	if(vpad_data.btn_hold & BUTTON_Y) 	{ key[kconf[3]] = 1; } else { key[kconf[3]] = 0; }
	if(vpad_data.btn_hold & BUTTON_LEFT) 	{ key[kconf[4]] = 1; } else { key[kconf[4]] = 0; }
	if(vpad_data.btn_hold & BUTTON_RIGHT) 	{ key[kconf[5]] = 1; } else { key[kconf[5]] = 0; }
	if(vpad_data.btn_hold & BUTTON_UP) 	{ key[kconf[6]] = 1; } else { key[kconf[6]] = 0; }
	if(vpad_data.btn_hold & BUTTON_DOWN) 	{ key[kconf[7]] = 1; } else { key[kconf[7]] = 0; }
	if(vpad_data.btn_hold & BUTTON_ZL) 	{ key[kconf[8]] = 1; } else { key[kconf[8]] = 0; }
	if(vpad_data.btn_hold & BUTTON_ZR) 	{ key[kconf[9]] = 1; } else { key[kconf[9]] = 0; }
	if(vpad_data.btn_hold & BUTTON_L) 	{ key[kconf[10]] = 1; } else { key[kconf[10]] = 0; }
	if(vpad_data.btn_hold & BUTTON_R) 	{ key[kconf[11]] = 1; } else { key[kconf[11]] = 0; }
	if(vpad_data.btn_hold & BUTTON_MINUS) 	{ key[kconf[12]] = 1; } else { key[kconf[12]] = 0; }
}
int sx,sy,rx,ry,cx,cy; //declare var here so we don't redeclare them each loop
void CHIP8_drawGraphics() {
	// Draw
	OSScreenClearBufferEx(0, 0x00000001);
	OSScreenClearBufferEx(1, 0x00000001);
	for(cy = 0; cy < 32; cy++)
	{
		for(cx = 0; cx < 64; cx++)
		{
			if(!(gfx[(cy*64) + cx] == 0)) {
				rx=(cx*13);
				ry=(cy*13);
				for(sx=0;sx<13;sx++){
					for(sy=0;sy<13;sy++){
						OSScreenPutPixelEx(0,(rx+sx),(ry+sy),0xffffff01); //Directly call WiiU function ( less calls (>10 mov and 3 jmp!) = more speed ;) )
						OSScreenPutPixelEx(1,(rx+sx),(ry+sy),0xffffff01);
					}
				}
			}		
		}
	}
	flipBuffers();
	CHIP8_drawFlag = false;
}
void kb_input() {
	kb_render();
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btn_hold & BUTTON_HOME) ExitApplication();
		if (vpad_data.btn_hold & BUTTON_A) return;
		if (vpad_data.tpdata.touched == 1) {
			if((vpad_data.tpdata1.x > 480) && (vpad_data.tpdata1.x < 764)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('z');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('a');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('q');
			} else if ((vpad_data.tpdata1.x > 764) && (vpad_data.tpdata1.x < 1028)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('x');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('s');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('w');
			} else if ((vpad_data.tpdata1.x > 1028) && (vpad_data.tpdata1.x < 1312)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('c');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('d');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('e');
			} else if ((vpad_data.tpdata1.x > 1312) && (vpad_data.tpdata1.x < 1582)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('v');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('f');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('r');
			} else if ((vpad_data.tpdata1.x > 1582) && (vpad_data.tpdata1.x < 1866)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('b');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('g');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('t');
			} else if ((vpad_data.tpdata1.x > 1866) && (vpad_data.tpdata1.x < 2126)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('n');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('h');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('y');
			} else if ((vpad_data.tpdata1.x > 2126) && (vpad_data.tpdata1.x < 2402)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('m');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('j');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('u');
			} else if ((vpad_data.tpdata1.x > 2402) && (vpad_data.tpdata1.x < 2688)) {
				if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('k');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('i');
			} else if ((vpad_data.tpdata1.x > 2688) && (vpad_data.tpdata1.x < 2952)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) kb_press('.');
				else if((vpad_data.tpdata1.y > 1222) && (vpad_data.tpdata1.y < 1760)) kb_press('l');
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('o');
			} else if ((vpad_data.tpdata1.x > 2952) && (vpad_data.tpdata1.x < 3250)) {
				if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)) kb_press('p');
			} else if ((vpad_data.tpdata1.x > 3308) && (vpad_data.tpdata1.x < 3712)) {
				if((vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 1222)) return;
				else if((vpad_data.tpdata1.y > 1760) && (vpad_data.tpdata1.y < 2236)){
					if (kb_text_len > 0) { //We dont't want a buffer underflow :P
 						kb_text_len--;
						kb_text[kb_text_len] = 0;
						kb_render();
						unsigned int dpc = 0x1000000;
						while(dpc--) ;
					}
				}
			}
		}
	}
}
void kb_render() {
	OSScreenClearBufferEx(1, 0x00000001);
	OSScreenPutFontEx(1, 20, 1, "==CHIP 8 EMULATOR==");
	OSScreenPutFontEx(1, 1, 3, "Please enter rom name");
	OSScreenPutFontEx(1, 1, 5, "ROM NAME: ");
	OSScreenPutFontEx(1, 11, 5, kb_text);
	OSScreenPutFontEx(1, 5, 8,  "q    w    e    r    t    y    u    i    o    p      <-");
	OSScreenPutFontEx(1, 5, 11, "a    s    d    f    g    h    j    k    l");
	OSScreenPutFontEx(1, 5, 14, "z    x    c    v    b    n    m         .           OK");
	OSScreenClearBufferEx(0, 0x00000001);
	OSScreenPutFontEx(0, 10, 1, "==CHIP 8 EMULATOR==");
	OSScreenPutFontEx(0, 0, 3, "Please enter rom name on the gamepad");
	OSScreenPutFontEx(0, 0, 5, "ROM NAME: ");
	OSScreenPutFontEx(0, 10, 5, kb_text);
	flipBuffers();
}
void kb_press(char kb_key) {
	kb_text[kb_text_len] = kb_key;
	kb_text_len++;
	kb_render();
	unsigned int d1 = 0x1000000;
	while(d1--) ;
}
void flipBuffers() {
	//Grab the buffer size for each screen (TV and gamepad)
	int buf0_size = OSScreenGetBufferSizeEx(0);
	int buf1_size = OSScreenGetBufferSizeEx(1);
	//Flush the cache
	DCFlushRange((void *)0xF4000000 + buf0_size, buf1_size);
	DCFlushRange((void *)0xF4000000, buf0_size);
	//Flip the buffer
	OSScreenFlipBuffersEx(0);
	OSScreenFlipBuffersEx(1);
}
void _osscreeninit() {
	void(*OSScreenInit)();
	OSDynLoad_FindExport(coreinit_handle, 0, "OSScreenInit", &OSScreenInit);
	//Call the Screen initilzation function.
	OSScreenInit();
	//Grab the buffer size for each screen (TV and gamepad)
	int buf0_size = OSScreenGetBufferSizeEx(0);
	int buf1_size = OSScreenGetBufferSizeEx(1);
	//Set the buffer area.
	OSScreenSetBufferEx(0, (void *)0xF4000000);
	OSScreenSetBufferEx(1, (void *)0xF4000000 + buf0_size);
}
void _osscreenexit() {
	int ii=0;
	for(ii;ii<2;ii++)
	{
		OSScreenClearBufferEx(0, 0x00000000);
		OSScreenClearBufferEx(1, 0x00000000);
		flipBuffers();
	}
}
int crand() {
  long seed = OSGetTime(); // the seed value for more randomnes
  seed = (seed * 32719 + 3) % 32749;
  return ((seed % 32719) + 1);
}
static unsigned long int rnd_rslt;
int rand() { //From: http://stackoverflow.com/questions/4768180/rand-implementation
    rnd_rslt = crand();
    rnd_rslt = rnd_rslt * 1103515245 + 12345;
    return (unsigned int)(rnd_rslt/65536) % 32768;
}
void ExitApplication() { //Thanks to Blackspoon (@GBATemp)
	_osscreenexit(); //clear buffers
   // Return to old stack for exit
   asm volatile("lis 1, 0x1ab5 ; ori 1, 1, 0xd138");

   unsigned int coreinit;
   OSDynLoad_Acquire("coreinit.rpl", &coreinit);
   void(*_Exit)();
   OSDynLoad_FindExport(coreinit, 0, "_Exit", &_Exit);

   _Exit();
}
void* memcopy(void* dst, const void* src, uint32_t size)
{
	uint32_t i;
	for (i = 0; i < size; i++)
		((uint8_t*) dst)[i] = ((const uint8_t*) src)[i];
	return dst;
}
void emu_settings() {
	emu_settings_render();
	unsigned int dbo = 0x2000000;
	while(dbo--) ; 
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btn_hold & BUTTON_HOME) ExitApplication();
		if (vpad_data.btn_hold & BUTTON_PLUS) {
			return;
		}
		if (vpad_data.tpdata.touched == 1) {
			if ((vpad_data.tpdata1.x > 265) && (vpad_data.tpdata1.x < 1766) && (vpad_data.tpdata1.y > 2450) && (vpad_data.tpdata1.y < 2836)) { //Delay between cycles
				
			} else if ((vpad_data.tpdata1.x > 265) && (vpad_data.tpdata1.x < 2175) && (vpad_data.tpdata1.y > 2050) && (vpad_data.tpdata1.y < 2450)) { //Change controller configuration
				emu_settings_kconf();
				emu_settings_render();
				//delay
			} else if ((vpad_data.tpdata1.x > 265) && (vpad_data.tpdata1.x < 1244) && (vpad_data.tpdata1.y > 1680) && (vpad_data.tpdata1.y < 2050)) return; //Close Settings
		}
	}
}
void emu_settings_render() {
	OSScreenClearBufferEx(1, 0x00000001);
	OSScreenPutFontEx(1, 20, 1, "==CHIP 8 EMULATOR==");
	OSScreenPutFontEx(1, 1, 3, "--SETTINGS--");
	OSScreenPutFontEx(1, 1, 5, "(Delay between cycles: )-not implemented yet");
	OSScreenPutFontEx(1, 1, 7, "Change controller configuration");
	OSScreenPutFontEx(1, 1, 9, "Close Settings");
	emu_settings_render_tv();
	flipBuffers();	
}
void emu_settings_render_tv() {
	OSScreenClearBufferEx(0, 0x00000001);
	OSScreenPutFontEx(0, 20, 1, "==CHIP 8 EMULATOR==");
	OSScreenPutFontEx(0, 1, 3, "--SETTINGS--");
	OSScreenPutFontEx(0, 1, 4, "(Delay between cycles: )-not implemented yet!");
	OSScreenPutFontEx(0, 1, 5, "Current controller configuration:");
	for (li=0;li<13;li++) {
		OSScreenPutFontEx(0, 1, 6+li, avaiable_keys[li]);
		OSScreenPutFontEx(0, 11, 6+li, emu_settings_getkey(kconf[li]));
	}
}
void emu_settings_kconf() {
	emu_settings_kconf_render();
	unsigned int dbe = 0x2000000;
	while(dbe--) ;
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btn_hold & BUTTON_HOME) ExitApplication();
		if (vpad_data.btn_hold & BUTTON_PLUS) return;
		if (vpad_data.tpdata.touched == 1) {
			emu_settings_kconf_render();
			if ((vpad_data.tpdata1.x > 334) && (vpad_data.tpdata1.x < 1700)) { //Touching one of the keys => change it's value
				if ((vpad_data.tpdata1.y > 2538) && (vpad_data.tpdata1.y < 2730)) { //A
					kconf[0]=set_kconf_key("A");
				} else if ((vpad_data.tpdata1.y > 2342) && (vpad_data.tpdata1.y < 2538)) { //B
					kconf[1]=set_kconf_key("B");
				} else if ((vpad_data.tpdata1.y > 2150) && (vpad_data.tpdata1.y < 2342)) { //X
					kconf[2]=set_kconf_key("X");
				} else if ((vpad_data.tpdata1.y > 1976) && (vpad_data.tpdata1.y < 2150)) { //Y
					kconf[3]=set_kconf_key("Y");
				} else if ((vpad_data.tpdata1.y > 1788) && (vpad_data.tpdata1.y < 1976)) { //LEFT
					kconf[4]=set_kconf_key("LEFT");
				} else if ((vpad_data.tpdata1.y > 1600) && (vpad_data.tpdata1.y < 1788)) { //RIGHT
					kconf[5]=set_kconf_key("RIGHT");
				} else if ((vpad_data.tpdata1.y > 1425) && (vpad_data.tpdata1.y < 1600)) { //UP
					kconf[6]=set_kconf_key("UP");
				} else if ((vpad_data.tpdata1.y > 1236) && (vpad_data.tpdata1.y < 1425)) { //DOWN
					kconf[7]=set_kconf_key("DOWN");
				} else if ((vpad_data.tpdata1.y > 1040) && (vpad_data.tpdata1.y < 1236)) { //ZL
					kconf[8]=set_kconf_key("ZL");
				} else if ((vpad_data.tpdata1.y > 860) && (vpad_data.tpdata1.y < 1040)) { //ZR
					kconf[9]=set_kconf_key("ZR");
				} else if ((vpad_data.tpdata1.y > 668) && (vpad_data.tpdata1.y < 860)) { //L
					kconf[10]=set_kconf_key("L");
				} else if ((vpad_data.tpdata1.y > 482) && (vpad_data.tpdata1.y < 668)) { //R
					kconf[11]=set_kconf_key("R");
				} else if ((vpad_data.tpdata1.y > 282) && (vpad_data.tpdata1.y < 482)) { //MINUS
					kconf[12]=set_kconf_key("MINUS");
				} else continue; //No key pressed
				emu_settings_kconf_render(); //A key was pressed: redraw the menu with updated key config
				unsigned int dbr = 0x4000000;
				while(dbr--) ; 
			} else if ((vpad_data.tpdata1.x > 3122) && (vpad_data.tpdata1.x < 3560) && (vpad_data.tpdata1.y > 2842) && (vpad_data.tpdata1.y < 3160)) return; //BACK
		}
	}
}
void emu_settings_kconf_render() {
	OSScreenClearBufferEx(1, 0x00000001);
	OSScreenPutFontEx(1, 20, 1, "==CHIP 8 EMULATOR==");
	OSScreenPutFontEx(1, 1, 3, "--CONTROLLER SETTINGS--");
	OSScreenPutFontEx(1, 52, 3, "-BACK-");
	for (li=0;li<13;li++) {
		OSScreenPutFontEx(1, 1, 5+li, avaiable_keys[li]);
		OSScreenPutFontEx(1, 11, 5+li, emu_settings_getkey(kconf[li]));
	}
	emu_settings_render_tv();
	flipBuffers();	
}
int set_kconf_key(char* key_to_change) {
	int value_to_return = 0x10;
	set_kconf_key_render(key_to_change);
	unsigned int dba = 0x2000000;
	while(dba--) ; 
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btn_hold & BUTTON_HOME) ExitApplication();
		if (vpad_data.btn_hold & BUTTON_PLUS) return 0x10;
		if (vpad_data.tpdata.touched == 1) {
			if ((vpad_data.tpdata1.x > 900) && (vpad_data.tpdata1.x < 1010)) { //1st column
				if ((vpad_data.tpdata1.y > 2250) && (vpad_data.tpdata1.y < 2610)) return 0x1;
				else if ((vpad_data.tpdata1.y > 1870) && (vpad_data.tpdata1.y < 2250)) return 0x4;
				else if ((vpad_data.tpdata1.y > 1500) && (vpad_data.tpdata1.y < 1870)) return 0x7;
				else if ((vpad_data.tpdata1.y > 1120) && (vpad_data.tpdata1.y < 1500)) return 0xA;
			} else if ((vpad_data.tpdata1.x > 1010) && (vpad_data.tpdata1.x < 1120)) { //2nd column
				if ((vpad_data.tpdata1.y > 2250) && (vpad_data.tpdata1.y < 2610)) return 0x2;
				else if ((vpad_data.tpdata1.y > 1870) && (vpad_data.tpdata1.y < 2250)) return 0x5;
				else if ((vpad_data.tpdata1.y > 1500) && (vpad_data.tpdata1.y < 1870)) return 0x8;
				else if ((vpad_data.tpdata1.y > 1120) && (vpad_data.tpdata1.y < 1500)) return 0x0;
			} else if ((vpad_data.tpdata1.x > 1120) && (vpad_data.tpdata1.x < 1230)) { //3rd column
				if ((vpad_data.tpdata1.y > 2250) && (vpad_data.tpdata1.y < 2610)) return 0x3;
				else if ((vpad_data.tpdata1.y > 1870) && (vpad_data.tpdata1.y < 2250)) return 0x6;
				else if ((vpad_data.tpdata1.y > 1500) && (vpad_data.tpdata1.y < 1870)) return 0x9;
				else if ((vpad_data.tpdata1.y > 1120) && (vpad_data.tpdata1.y < 1500)) return 0xB;
			} else if ((vpad_data.tpdata1.x > 1230) && (vpad_data.tpdata1.x < 1340)) { //4th column
				if ((vpad_data.tpdata1.y > 2250) && (vpad_data.tpdata1.y < 2610)) return 0xC;
				else if ((vpad_data.tpdata1.y > 1870) && (vpad_data.tpdata1.y < 2250)) return 0xD;
				else if ((vpad_data.tpdata1.y > 1500) && (vpad_data.tpdata1.y < 1870)) return 0xE;
				else if ((vpad_data.tpdata1.y > 1120) && (vpad_data.tpdata1.y < 1500)) return 0xF;
			} 
			if ((vpad_data.tpdata1.x > 942) && (vpad_data.tpdata1.x < 1230) && (vpad_data.tpdata1.y > 600) && (vpad_data.tpdata1.y < 920)) return 0x10;
		}
	}
}
void set_kconf_key_render(char* key_to_change) {
	OSScreenClearBufferEx(1, 0x00000001);
	OSScreenPutFontEx(1, 20, 1, "==CHIP 8 EMULATOR==");
	char ass_key_str[50];
	__os_snprintf(ass_key_str, 50, "WHAT CHIP8 KEY SHOULD BE ASSIGNED TO %s ?", key_to_change);
	OSScreenPutFontEx(1, 10, 3, ass_key_str);
	OSScreenPutFontEx(1, 10, 5,  "+-+-+-+-+");
	OSScreenPutFontEx(1, 10, 6,  "|1|2|3|C|");
	OSScreenPutFontEx(1, 10, 7,  "+-+-+-+-+");
	OSScreenPutFontEx(1, 10, 8,  "|4|5|6|D|");
	OSScreenPutFontEx(1, 10, 9,  "+-+-+-+-+");
	OSScreenPutFontEx(1, 10, 10, "|7|8|9|E|");
	OSScreenPutFontEx(1, 10, 11, "+-+-+-+-+");
	OSScreenPutFontEx(1, 10, 12, "|A|0|B|F|");
	OSScreenPutFontEx(1, 10, 13, "+-+-+-+-+");
	OSScreenPutFontEx(1, 12, 15, "NONE");
	emu_settings_render_tv();
	flipBuffers();
}
char* getkey;
char* emu_settings_getkey(int hexval) {
	switch(hexval) {
		case 0x0:
			getkey="Keypad 0";
			break;
		case 0x1:
			getkey="Keypad 1";
			break;
		case 0x2:
			getkey="Keypad 2";
			break;
		case 0x3:
			getkey="Keypad 3";
			break;
		case 0x4:
			getkey="Keypad 4";
			break;
		case 0x5:
			getkey="Keypad 5";
			break;
		case 0x6:
			getkey="Keypad 6";
			break;
		case 0x7:
			getkey="Keypad 7";
			break;
		case 0x8:
			getkey="Keypad 8";
			break;		
		case 0x9:
			getkey="Keypad 9";
			break;	
		case 0xA:
			getkey="Keypad A";
			break;	
		case 0xB:
			getkey="Keypad B";
			break;	
		case 0xC:
			getkey="Keypad C";
			break;	
		case 0xD:
			getkey="Keypad D";
			break;
		case 0xE:
			getkey="Keypad E";
			break;	
		case 0xF:
			getkey="Keypad F";
			break;	
		case 0x10:
			getkey="Not assigned";
			break;	
		default:
			getkey="??";
			break;	
	}
	return getkey;
}
