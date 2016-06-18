#include "CHIP8.h"

char* avaiable_keys[13]={"A","B","X","Y","LEFT","RIGHT","UP","DOWN","ZL","ZR","L","R","MINUS"}; //Avaiable WiiU keys
int kconf[13]={0x5,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10}; //Default key configuration
char rom[2048]; //CHIP8 roms shouldn't be more than 2Kb and also, why caring about stability?

void CHIP8_initialize() {
	pc     = 0x200;  // Program counter starts at 0x200
	opcode = 0;      // Reset current opcode	
	I      = 0;      // Reset index register
	sp     = 0;      // Reset stack pointer
	memset(gfx,0,2048); //Clear display
	memset(cstack,0,16); //Clear stack
	memset(key,0,17); //Reset keys
	memset(memory,0,4096); //Clear memory
	memcpy(&memory[512],rom,2048*sizeof(char)); //Load rom into memory
	memcpy(memory,chip8_fontset,80*sizeof(char)); //Load fontset
	//Reset timers
	delay_timer = 0;
	sound_timer = 0;
}

void LoadROM() {
	UnmountVirtualPaths(); 
	MountVirtualDevices();
	DIR_P *dir = vrt_opendir("/sd/wiiu/apps/CHIP8/roms/", "."); //Path of roms
	struct dirent *dirent = NULL; //Struct used for storing current file/dir
	char roms[16][256]; //Array of roms in /sd/wiiu/apps/CHIP8/roms/
	int fnum = 0; //Number of roms
	while ((dirent = vrt_readdir(dir)) != 0 && fnum<16) { //Read al dirs and check that the roms are not more than 16
		if (check_extension(dirent->d_name)) { //Check that the file has .c8/.C8 extension
			__os_snprintf(roms[fnum], 256, "%s", dirent->d_name); //Print the rom name to the roms array
			fnum++; //Increase files number
		}
	}
	int sel_file = 0; //Current selected file
	rom_choose_render(roms, fnum, sel_file); //First render
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btns_h & VPAD_BUTTON_HOME) {
			exitpg = true;
			goto end;
		}
		if (vpad_data.btns_h & VPAD_BUTTON_UP) { //Scroll UP :P
			if (sel_file>0) sel_file--;
			else (sel_file=(fnum-1));
			rom_choose_render(roms, fnum, sel_file);
			msleep(300); //debounce
		}
		if (vpad_data.btns_h & VPAD_BUTTON_DOWN) { //Scroll DOWN :P
			if (sel_file<(fnum-1)) sel_file++;
			else (sel_file=0);
			rom_choose_render(roms, fnum, sel_file);
			msleep(300); //debounce
		}
		if (vpad_data.btns_h & VPAD_BUTTON_A) { //A file has ben selected
			break;
			msleep(300); //debounce
		}
	}
        FILE *f=vrt_fopen("/sd/wiiu/apps/CHIP8/roms/", roms[sel_file], "rb");
	fread(rom, 1, 2048, f);
end: ;
}
bool check_extension(char* str) { //Check that the file have .c8/.C8 extension
	const char *ext = &str[strlen(str)-3]; //Take last 3 char
	if (strcmp(ext,".c8")==0 || strcmp(ext,".C8")==0) return true; //Check last three char
	else return false; //Char are not .c8/.C8 or something very strange happened
}
void rom_choose_render(char files[16][256], int fnum, int selected) { //Render the rom browser
	OSScreenClearBufferEx(1, 0x00000001); //Clear DRC screen
	OSScreenPutFontEx(1, 20, 0, "==CHIP 8 EMULATOR=="); 
	OSScreenPutFontEx(1, 1, 1, "--PLEASE SELECT A ROM--");
	for(int i=0;i<fnum;i++) OSScreenPutFontEx(1, 4, i+2, files[i]); //Print all the roms in files

	OSScreenPutFontEx(1, 1, selected+2, "->"); //Print the cursor
	rom_choose_render_tv(files, fnum, selected);
	flipBuffers();	
}
void rom_choose_render_tv(char files[16][256], int fnum, int selected) { //Render the rom browser
	OSScreenClearBufferEx(0, 0x00000001); //Clear TV screen
	OSScreenPutFontEx(0, 20, 0, "==CHIP 8 EMULATOR=="); 
	OSScreenPutFontEx(0, 1, 1, "--PLEASE SELECT A ROM--");
	for(int i=0;i<fnum;i++) OSScreenPutFontEx(0, 4, i+2, files[i]); //Print all the roms in files
	OSScreenPutFontEx(1, 1, selected+2, "->"); //Print the cursor
}

void emu_settings() {
	emu_settings_render();
	msleep(300); //debounce
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btns_h & VPAD_BUTTON_HOME) {exitpg = true; return;}
		if (vpad_data.btns_h & VPAD_BUTTON_PLUS) {
			msleep(300); //debounce
			return;
		}
		if (vpad_data.tpdata.touched == 1) {
			if ((vpad_data.tpdata1.x > 265) && (vpad_data.tpdata1.x < 1766) && (vpad_data.tpdata1.y > 2450) && (vpad_data.tpdata1.y < 2836)) { //Delay between cycles
				
			} else if ((vpad_data.tpdata1.x>265)&&(vpad_data.tpdata1.x<2175)&&(vpad_data.tpdata1.y>2050)&&(vpad_data.tpdata1.y<2450)) { //Change controller configuration
				emu_settings_kconf();
				emu_settings_render();
				msleep(300); //debounce
			} else if ((vpad_data.tpdata1.x>265)&&(vpad_data.tpdata1.x<1244)&&(vpad_data.tpdata1.y>1680)&&(vpad_data.tpdata1.y<2050))return; //Close Settings
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
	for(int i=0;i<13;i++) {
		OSScreenPutFontEx(0, 1, 6+i, avaiable_keys[i]);
		OSScreenPutFontEx(0, 11, 6+i, emu_settings_getkey(kconf[i]));
	}
}
void emu_settings_kconf() {
	emu_settings_kconf_render();
	msleep(300); //debounce
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btns_h & VPAD_BUTTON_HOME) return;
		if (vpad_data.btns_h & VPAD_BUTTON_PLUS) return;
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
				msleep(300); //debounce
			} else if ((vpad_data.tpdata1.x > 3122) && (vpad_data.tpdata1.x < 3560) && (vpad_data.tpdata1.y > 2842) && (vpad_data.tpdata1.y < 3160)) return; //BACK
		}
	}
}
void emu_settings_kconf_render() {
	OSScreenClearBufferEx(1, 0x00000001);
	OSScreenPutFontEx(1, 20, 1, "==CHIP 8 EMULATOR==");
	OSScreenPutFontEx(1, 1, 3, "--CONTROLLER SETTINGS--");
	OSScreenPutFontEx(1, 52, 3, "-BACK-");
	for(int i=0;i<13;i++) {
		OSScreenPutFontEx(1, 1, 5+i, avaiable_keys[i]);
		OSScreenPutFontEx(1, 11, 5+i, emu_settings_getkey(kconf[i]));
	}
	emu_settings_render_tv();
	flipBuffers();	
}
int set_kconf_key(char* key_to_change) {
	set_kconf_key_render(key_to_change);
	msleep(300); //debounce
	for(;;) {
		VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
		if (vpad_data.btns_h & VPAD_BUTTON_HOME) return 0x10;
		if (vpad_data.btns_h & VPAD_BUTTON_PLUS) return 0x10;
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
