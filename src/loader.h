#ifndef LOADER_H
#define LOADER_H

#include "types.h"
#include "vpad.h"

void _main();
void CHIP8_initialize();
void CHIP8_loadGame();
void CHIP8_emulateCycle();
void CHIP8_setKeys();
void CHIP8_drawGraphics();
static int curl_write_data_callback(void *buffer, int size, int nmemb, void *userp);
void _mainThread(int argc, void *argv);
static int curl_download_file(void *curl, char *url);
void* memcopy(void* dst, const void* src, uint32_t size);
void flipBuffers();
void _osscreeninit();
void _osscreenexit();
int crand();
int rand();
void beep_sound();
void ExitApplication();
void kb_input();
void kb_render();
void kb_press(char kb_key);
void emu_settings();
void emu_settings_render();
void emu_settings_render_tv();
void emu_settings_kconf();
void emu_settings_kconf_render();
int set_kconf_key(char* key_to_change);
void set_kconf_key_render(char* key_to_change);
char* emu_settings_getkey(int hexval);


char kb_text[256];
int kb_text_len = 0;
char* avaiable_keys[13]={"A","B","X","Y","LEFT","RIGHT","UP","DOWN","ZL","ZR","L","R","MINUS"};
char* fname;

/* Default key config:
0=A		| 0x0=keypad 0 | 0xD=keypad D
1=B		| 0x1=keypad 1 | 0xE=keypad E
2=X		| 0x2=keypad 2 | 0xF=keypad F
3=Y		| 0x3=keypad 3 | 0x10=unassigned
4=LEFT		| 0x4=keypad 4 |
5=RIGHT		| 0x5=keypad 5 | 
6=UP		| 0x6=keypad 6 | 
7=DOWN		| 0x7=keypad 7 | 
8=ZL		| 0x8=keypad 8 | 
9=ZR		| 0x9=keypad 9 | 
10=L		| 0xA=keypad A | 
11=R		| 0xB=keypad B | 
12=MINUS	| 0xC=keypad C | 
*/
int kconf[13]={0x5,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10};

//--HANDLES--
uint32_t nn_ac_handle, nsysnet_handle, libcurl_handle, nn_startupid, vpadHandle;

//--WiiU Functions Prototipes--
int(*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
void(*DCFlushRange)(void *buffer, unsigned int length);
unsigned int(*OSScreenFlipBuffersEx)(unsigned int bufferNum);
unsigned int(*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
unsigned int(*OSScreenPutPixelEx)(unsigned int bufferNum, unsigned int posX, unsigned int posY, uint32_t color);
unsigned int(*OSScreenPutFontEx)(unsigned int bufferNum, unsigned int posX, unsigned int line, void * buffer);
unsigned int(*OSScreenClearBufferEx)(unsigned int bufferNum, unsigned int temp);
unsigned int(*OSScreenGetBufferSizeEx)(unsigned int bufferNum);
unsigned int(*OSScreenSetBufferEx)(unsigned int bufferNum, void * addr);
int(*VPADRead)(int controller, VPADData *buffer, unsigned int num, int *error);
int64_t (*OSGetTime)();


/* Alloc */
void* (*MEMAllocFromDefaultHeap)(unsigned int size);
#define malloc (*MEMAllocFromDefaultHeap)

//--CURL--
void*(*OSAllocFromSystem)(uint32_t size, int align);
void(*OSFreeToSystem)(void *ptr);
int (*OSCreateThread)(void *thread, void *entry, int argc, void *args, uint32_t stack,
uint32_t stack_size, int priority, uint16_t attr);
int (*OSResumeThread)(void *thread);
int (*OSIsThreadTerminated)(void *thread);
int  (*ACInitialize)(void);
int  (*ACGetStartupId)(uint32_t *id);
int  (*ACConnectWithConfigId)(uint32_t id);
int  (*socket_lib_init)(void);
int  (*curl_global_init)(int opts);
void*(*curl_easy_init)(void);
void (*curl_easy_cleanup)(void *handle);;
void (*curl_easy_setopt) (void *handle, uint32_t param, void *op);
int  (*curl_easy_perform)(void *handle);
void (*curl_easy_getinfo)(void *handle, uint32_t param, void *info);
void (*OSDynLoad_Release)(uint32_t handle);

#define CURL_GLOBAL_SSL (1<<0)
#define CURL_GLOBAL_WIN32 (1<<1)
#define CURL_GLOBAL_ALL (CURL_GLOBAL_SSL|CURL_GLOBAL_WIN32)
#define CURL_GLOBAL_NOTHING 0
#define CURL_GLOBAL_DEFAULT CURL_GLOBAL_ALL
#define OSTHREAD_SIZE	0x1000
//Copied from the ELF loader.
#define CURLOPT_URL 10002
#define CURLOPT_WRITEFUNCTION 20011
#define CURLINFO_RESPONSE_CODE 0x200002



#endif /* LOADER_H */
