#include "Application.h"
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/gx2_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/padscore_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "dynamic_libs/ax_functions.h"
#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "common/common.h"

unsigned char gfx[128][64]; //Screen
u32 renderTexture[256 * 128];  // comment out if using a gd image for the playfield instead and uncomment the gd image below
//gdImagePtr virtualDisp;  // uncomment if using a gd image for the playfield and comment out the renderTexture above
bool CHIP8_drawFlag;
bool CHIP8_soundFlag;
bool emulateROM;
int controlUsed;


int _entryPoint() {
    //!*******************************************************************
    //!                   Initialize function pointers                   *
    //!*******************************************************************
    //! do OS (for acquire) and sockets first so we got logging
    InitOSFunctionPointers();
    InitSocketFunctionPointers();

    log_init("192.168.1.100");
    log_print("Starting launcher\n");

    InitFSFunctionPointers();
    InitGX2FunctionPointers();
    InitSysFunctionPointers();
    InitVPadFunctionPointers();
    InitPadScoreFunctionPointers();
    InitAXFunctionPointers();

    log_print("Function exports loaded\n");

    //!*******************************************************************
    //!                    Initialize heap memory                        *
    //!*******************************************************************
    log_print("Initialize memory management\n");
    memoryInitialize();

    //!*******************************************************************
    //!                        Initialize FS                             *
    //!*******************************************************************
    log_printf("Mount SD partition\n");
    mount_sd_fat("sd");

	VPADInit(); //Init VPAD

//    virtualDisp = 0;  // uncomment if using a gd image for the playfield
//    
//    virtualDisp = gdImageCreateTrueColor(256, 128);
    
    CHIP8_soundFlag = false;
    emulateROM = false;

    //!*******************************************************************
    //!                    Enter main application                        *
    //!*******************************************************************
    log_printf("Start main application\n");
    Application::instance()->exec();

    log_printf("Main application stopped\n");

    Application::destroyInstance();

    log_printf("Unmount SD\n");
    unmount_sd_fat("sd");
    log_printf("Release memory\n");
    memoryRelease();
    log_deinit();
	return 0;
}
