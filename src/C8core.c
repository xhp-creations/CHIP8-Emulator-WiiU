#include "C8core.h"

unsigned short opcode; //Current opcode
unsigned char memory[4096]; 

/* CHIP8 memory (4K):
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM */

unsigned char V[16]; //CHIP8 Registers (1 reg=8 bit=1 byte=char)
unsigned short I; //Index register
unsigned short pc; //CHIP8 Program counter
unsigned char delay_timer; //CHIP8 Delay timer
unsigned char sound_timer; //CHIP8 Sound timer
unsigned short cstack[16]; //CHIP8 Stack
unsigned short sp;
unsigned char key[0x11]; //State of buttons; we assign 1 more='no button assigned'

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


bool keyPress = false;
int xline,yline,curr_key;
unsigned short kpixel,kheight,kx,ky;

void CHIP8_emulateCycle() {
  opcode = memory[pc] << 8 | memory[pc + 1]; //Fetch opcode
  switch (opcode & 0xF000) { //Decode opcode
    case 0x0000:
      switch (opcode & 0x000F) {
        case 0x0000:
          memset(gfx, 0, 2048);
          CHIP8_drawFlag = true;
          pc += 2;
          break;
        case 0x000E:
          sp--;
          pc = cstack[sp];
          pc += 2;
          break;
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
      if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 4;
      else pc += 2;
      break;
    case 0x4000:
      if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 4;
      else pc += 2;
      break;
    case 0x5000:
      if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) pc += 4;
      else pc += 2;
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
      switch (opcode & 0x000F) {
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
          if (V[(opcode & 0x00F0) >> 4] > (0xFF - V[(opcode & 0x0F00) >> 8])) V[0xF] = 1;
          else V[0xF] = 0;
          V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;
        case 0x0005:
          if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) V[0xF] = 0;
          else V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
          pc += 2;
          break;
        case 0x0006:
          V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x1;
          V[(opcode & 0x0F00) >> 8] >>= 1;
          pc += 2;
          break;
        case 0x0007:
          if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) V[0xF] = 0;
          else V[0xF] = 1;
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
          pc += 2;
          break;
        case 0x000E:
          V[0xF] = V[(opcode & 0x0F00) >> 8] >> 7;
          V[(opcode & 0x0F00) >> 8] <<= 1;
          pc += 2;
          break;
      }
      break;
    case 0x9000:
      if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) pc += 4;
      else pc += 2;
      break;
    case 0xA000:
      I = opcode & 0x0FFF;
      pc += 2;
      break;
    case 0xB000:
      pc = (opcode & 0x0FFF) + V[0];
      break;
    case 0xC000:
      V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
      pc += 2;
      break;
    case 0xD000: {
        kx = V[(opcode & 0x0F00) >> 8];
        ky = V[(opcode & 0x00F0) >> 4];
        kheight = opcode & 0x000F;
        V[0xF] = 0;
        for (yline = 0; yline < kheight; yline++) {
          kpixel = memory[I + yline];
          for (xline = 0; xline < 8; xline++) {
            if ((kpixel & (0x80 >> xline)) != 0) {
              if (gfx[(kx + xline + ((ky + yline) * 64))] == 1) V[0xF] = 1;
              gfx[kx + xline + ((ky + yline) * 64)] ^= 1;
            }
          }
        }
        CHIP8_drawFlag = true;
        pc += 2;
      }
      break;
    case 0xE000:
      switch (opcode & 0x00FF) {
        case 0x009E:
          if (key[V[(opcode & 0x0F00) >> 8]] != 0) pc += 4;
          else pc += 2;
          break;
        case 0x00A1:
          if (key[V[(opcode & 0x0F00) >> 8]] == 0) pc += 4;
          else pc += 2;
          break;
      }
      break;
    case 0xF000:
      switch (opcode & 0x00FF) {
        case 0x0007:
          V[(opcode & 0x0F00) >> 8] = delay_timer;
          pc += 2;
          break;
        case 0x000A: {
            for (curr_key = 0; curr_key < 16; curr_key++) {
              if (key[curr_key] != 0) {
                V[(opcode & 0x0F00) >> 8] = curr_key;
                keyPress = true;
              }
            }
            if (!keyPress) return;
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
          if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF) V[0xF] = 1;
          else V[0xF] = 0;
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
          //for (li = 0; li <= ((opcode & 0x0F00) >> 8); li++) memory[I + li] = V[li];
	  memcpy(&memory[I],V,(((opcode & 0x0F00) >> 8)+1)*sizeof(unsigned char));
          I += ((opcode & 0x0F00) >> 8) + 1;
          pc += 2;
          break;
        case 0x0065:
          //for (lj = 0; lj <= ((opcode & 0x0F00) >> 8); lj++) V[lj] = memory[I + lj];
	  memcpy(V, &memory[I], (((opcode & 0x0F00) >> 8)+1)*sizeof(unsigned char));
          I += ((opcode & 0x0F00) >> 8) + 1;
          pc += 2;
          break;
      }
      break;
  }
  if (delay_timer > 0) delay_timer--; // Update timers
  if (sound_timer > 0) {
    //if (sound_timer == 1) beep(); //Soon(tm)
    sound_timer--;
  }
}

void CHIP8_setKeys(){
	VPADRead(0, &vpad_data, 1, &error); //Read the vpaddata
	if (vpad_data.btns_h & VPAD_BUTTON_HOME) exitpg=true;
	if (vpad_data.btns_h & VPAD_BUTTON_PLUS) emu_settings();

	if(vpad_data.btns_h & VPAD_BUTTON_A) 	{ key[kconf[0]] = 1; } else { key[kconf[0]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_B) 	{ key[kconf[1]] = 1; } else { key[kconf[1]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_X) 	{ key[kconf[2]] = 1; } else { key[kconf[2]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_Y) 	{ key[kconf[3]] = 1; } else { key[kconf[3]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_LEFT) 	{ key[kconf[4]] = 1; } else { key[kconf[4]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_RIGHT) 	{ key[kconf[5]] = 1; } else { key[kconf[5]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_UP) 	{ key[kconf[6]] = 1; } else { key[kconf[6]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_DOWN) 	{ key[kconf[7]] = 1; } else { key[kconf[7]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_ZL) 	{ key[kconf[8]] = 1; } else { key[kconf[8]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_ZR) 	{ key[kconf[9]] = 1; } else { key[kconf[9]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_L) 	{ key[kconf[10]] = 1; } else { key[kconf[10]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_R) 	{ key[kconf[11]] = 1; } else { key[kconf[11]] = 0; }
	if(vpad_data.btns_h & VPAD_BUTTON_MINUS) 	{ key[kconf[12]] = 1; } else { key[kconf[12]] = 0; }
}
/*
int crand() {
  long seed = OSGetTime(); // the seed value for more randomnes
  seed = (seed * 32719 + 3) % 32749;
  return ((seed % 32719) + 1);
}
*/
