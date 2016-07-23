#include "C8core.h"

unsigned short opcode; //Current opcode
unsigned char memory[4096]; 

/* CHIP8 memory (4K):
0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
0x200-0xFFF - Program ROM and work RAM */

unsigned char V[16]; //CHIP8 Registers (1 reg=8 bit=1 byte=char)
unsigned char hp48_flags[8]; //SUPER-CHIP8 HP48 Flags
unsigned short I; //Index register
unsigned short pc; //CHIP8 Program counter
unsigned char delay_timer; //CHIP8 Delay timer
unsigned char sound_timer; //CHIP8 Sound timer
unsigned short cstack[16]; //CHIP8 Stack
unsigned short sp;
unsigned char key[0x11]; //State of buttons; we assign 1 more='no button assigned'
int mode;

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

unsigned char bigfont[16*10] = {
    0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF,	// 0
    0x18, 0x78, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0xFF,	// 1
    0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF,	// 2
    0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF,	// 3
    0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, // 4
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF,	// 5
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF,	// 6
    0xFF, 0xFF, 0x03, 0x03, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18, // 7
    0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF,	// 8
    0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF,	// 9
    0x7E, 0xFF, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xC3, // A
    0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, // B
    0x3C, 0xFF, 0xC3, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xFF, 0x3C, // C
    0xFC, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFE, 0xFC, // D
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // E
    0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0  // F
};

bool keyPress = false;
int xline,yline,curr_key;
unsigned short kpixel,kheight,kx,ky;

void drawSprite(unsigned char X, unsigned char Y, unsigned char N)
{
    V[0xF] = 0;
    switch (mode)
    {
        case 0: // CHIP-8 mode
            if (N == 0) N = 16;
            for (int yline = 0; yline < N; yline++)
            {
                unsigned char data = memory[I + yline];
                for (int xpix = 0; xpix < 8; xpix++)
                {
                    if((data & (0x80 >> xpix)) != 0)
                    {
                        if ((V[X] + xpix) < 64 && (V[Y] + yline) < 32 && (V[X] + xpix) >= 0 && (V[Y] + yline) >= 0)
                        {
                            if (gfx[(V[X] + xpix)*2][(V[Y] + yline)*2] == 1) V[0xF] = 1;
                            gfx[(V[X] + xpix)*2][(V[Y] + yline)*2] ^= 1;
                            gfx[(V[X] + xpix)*2 + 1][(V[Y] + yline)*2] ^= 1;
                            gfx[(V[X] + xpix)*2][(V[Y] + yline)*2 + 1] ^= 1;
                            gfx[(V[X] + xpix)*2 + 1][(V[Y] + yline)*2 + 1] ^= 1;
                        }
                    }
                }
            }
            break;
            
        case 1: // SCHIP mode
            if (N == 0) 
                for (int yline = 0; yline < 16; yline++)
                {
                    unsigned char data = memory[I + yline*2];
                    for (int xpix = 0; xpix < 8; xpix++)
                    {
                        if((data & (0x80 >> xpix)) != 0)
                        {
                            if ((V[X] + xpix) < 128 && (V[Y] + yline) < 64 && (V[X] + xpix) >= 0 && (V[Y] + yline) >= 0)
                            {
                                if (gfx[V[X] + xpix][V[Y] + yline] == 1) V[0xF] = 1;
                                gfx[V[X] + xpix][V[Y] + yline] ^= 1;
                            }
                        }
                    }
                    data = memory[I + 1 + yline*2];
                    for (int xpix = 0; xpix < 8; xpix++)
                    {
                        if((data & (0x80 >> xpix)) != 0)
                        {
                            if ((V[X] + xpix + 8) < 128 && (V[Y] + yline) < 64 && (V[X] + xpix + 8) >= 0 && (V[Y] + yline) >= 0)
                            {
                                if (gfx[V[X] + xpix + 8][V[Y] + yline] == 1) V[0xF] = 1;
                                gfx[V[X] + xpix + 8][V[Y] + yline] ^= 1;
                            }
                        }
                    }
                }
            else
                for (int yline = 0; yline < N; yline++)
                {
                    unsigned char data = memory[I + yline];
                    for (int xpix = 0; xpix < 8; xpix++)
                    {
                        if((data & (0x80 >> xpix)) != 0)
                        {
                            if ((V[X] + xpix) < 128 && (V[Y] + yline) < 64 && (V[X] + xpix) >= 0 && (V[Y] + yline) >= 0)
                            {
                                if (gfx[V[X] + xpix][V[Y] + yline] == 1) V[0xF] = 1;
                                gfx[V[X] + xpix][V[Y] + yline] ^= 1;
                            }
                        }
                    }
                }
            break;
    }
}

void CHIP8_emulateCycle() {
    opcode = memory[pc] << 8 | memory[pc + 1]; //Fetch opcode
    pc += 2;
    switch (opcode & 0xF000) { // Decode opcode
    case 0x0000:
      /*
        00CN - SCD N
        Scroll display NIBBLE lines down
        SCHIP-8 instruction to scroll display NIBBLE lines down.
      */
      if ((opcode & 0x00F0)>>4 == 0xC)
      {
        int N = opcode & 0x000F;
        for (int y = 63; y > N; y--)
            for (int x = 0; x < 128; x++)
                gfx[x][y] = gfx[x][y-N];
        for (int y = 0; y < N; y++)
            for (int x = 0; x < 128; x++)
                gfx[x][y] = 0;
        break;
      }
      switch (opcode & 0x00FF) {
      /*
        00E0 - CLS
        Clear the display.
      */
        case 0x00E0:
          memset(gfx, 0, 8192);
          CHIP8_drawFlag = true;
          break;
      /*
        00EE - RET
        Return from a subroutine.
        The interpreter sets the program counter to the address at
        the top of the stack, then subtracts 1 from the stack
        pointer.
      */
        case 0x00EE:
          pc = cstack[--sp];
          break;
      /*
        00FB - SCR
        Scroll display right
        SCHIP-8 instruction to scroll display 4 pixels to the
        right.
      */
        case 0x00FB:
            for (int y = 0; y < 64; y++)
            {
                for (int x = 127; x > 3; x--)
                gfx[x][y] = gfx[x-4][y];
                gfx[0][y] = 0;
                gfx[1][y] = 0;
                gfx[2][y] = 0;
                gfx[3][y] = 0;
            }
            break;
      /*
        00FC - SCL
        Scroll display left
        SCHIP-8 instruction to scroll display 4 pixels to the
        left.
      */
        case 0x00FC:
            for (int y = 0; y < 64; y++)
            {
                for (int x = 0; x < 124; x++)
                gfx[x][y] = gfx[x+4][y];
                gfx[124][y] = 0;
                gfx[125][y] = 0;
                gfx[126][y] = 0;
                gfx[127][y] = 0;
            }
            break;
      /*
        00FE - LOW
        Enable low res (64x32) mode
        SCHIP-8 instruction to enable default / low res mode.
      */
        case 0xFE:
            mode = 0;
            break;
      /*
        00FF - HIGH
        Enable high res (128x64) mode
        SCHIP-8 instruction to enable high res mode.
      */
        case 0xFF:
            mode = 1;
            break;
      }
      break;
      /* 0x1nnn : jmp nnn : jump to address nnn */
    case 0x1000:
      pc = opcode & 0x0FFF;
      break;
      /*
        2nnn - CALL addr
        Call subroutine at nnn.
        The interpreter increments the stack pointer, then puts the current PC
        on the top of the stack. The PC is then set to nnn.
      */
    case 0x2000:
      cstack[sp++] = pc;
      pc = opcode & 0x0FFF;
      break;
      /*
        3xNN - SE VX, byte
        Skip next instruction if VX = NN.
        The interpreter compares register VX to NN, and if they are
        equal, increments the program counter by 2.
      */
    case 0x3000:
      if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) pc += 2;
      break;
      /*
        4xNN - SNE VX, byte
        Skip next instruction if VX != NN.
        The interpreter compares register VX to NN, and if they are
        not equal, increments the program counter by 2.
      */
    case 0x4000:
      if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) pc += 2;
      break;
      /*
        5XY0 - SE VX, VY
        Skip next instruction if VX = VY.
        The interpreter compares register VX to register VY, and if
        they are equal, increments the program counter by 2.
      */
    case 0x5000:
      if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4]) pc += 2;
      break;
      /*
        6XNN - LD VX, NN
        Set VX = NN.
        The interpreter puts the value NN into register VX.
      */
    case 0x6000:
      V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
      break;
      /*
        7xNN - ADD VX, byte
        Set VX = VX + NN.
        Adds the value NN to the value of register VX, then stores the
        result in VX.
      */
    case 0x7000:
      V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
      break;
    case 0x8000:
      switch (opcode & 0x000F) {
          /*
            8XY0 - LD VX, VY
            Set VX = VY.
            Stores the value of register VY in register VX.
          */
        case 0x0000:
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
          break;
          /*
            8XY1 - OR VX, VY
            Set VX = VX OR VY.
            Performs a bitwise OR on the values of VX and VY, then stores
            the result in VX. A bitwise OR compares the corrseponding bits
            from two values, and if either bit is 1, then the same bit in
            the result is also 1. Otherwise, it is 0.
          */
        case 0x0001:
          V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
          break;
          /*
            8XY2 - AND VX, VY
            Set VX = VX AND VY.
            Performs a bitwise AND on the values of VX and VY, then stores the
            result in VX. A bitwise AND compares the corrseponding bits from two
            values, and if both bits are 1, then the same bit in the result is
            also 1. Otherwise, it is 0.
          */
        case 0x0002:
          V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
          break;
          /*
            8XY3 - XOR VX, VY
            Set VX = VX XOR VY.
            Performs a bitwise exclusive OR on the values of VX and
            VY, then stores the result in VX. An exclusive OR
            compares the corrseponding bits from two values, and if
            the bits are not both the same, then the corresponding
            bit in the result is set to 1. Otherwise, it is 0.
          */
        case 0x0003:
          V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
          break;
          /*
            8XY4 - ADD VX, VY
            Set VX = VX + VY, set VF = carry.
            The values of VX and VY are added together. If the result is
            greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise
            0. Only the lowest 8 bits of the result are kept, and stored
            in VX.
          */
        case 0x0004:;
          int i;
          i = (int)(V[((opcode & 0x0F00)>>8)]) + (int)(V[((opcode & 0x00F0)>>4)]);
          if (i > 255)
              V[0xF] = 1;
          else
              V[0xF] = 0;
          V[((opcode & 0x0F00)>>8)] = i;
          break;
          /*
            8XY5 - SUB VX, VY
            Set VX = VX - VY, set VF = NOT borrow.
            If VX > VY, then VF is set to 1, otherwise 0. Then VY is
            subtracted from VX, and the results stored in VX.
          */
        case 0x0005:
          if (V[(opcode & 0x0F00) >> 8] >= V[(opcode & 0x00F0) >> 4]) V[0xF] = 1;
          else V[0xF] = 0;
          V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
          break;
          /*
            8XY6 - SHR VX {, VY}
            Originally: Set VX = VY SHR 1
            Today: Set VX = VX SHR 1
            If the least-significant bit of VY is 1, then VF is set to 1,
            otherwise 0. Then VY is shifted right by 1 and stored in VX.
          */
        case 0x0006:
          V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x01;
          V[(opcode & 0x0F00) >> 8] >>= 1;
          break;
          /*
            8XY7 - SUBN VX, VY
            Set VX = VY - VX, set VF = NOT borrow.
            If VY > VX, then VF is set to 1, otherwise 0. Then VX is
            subtracted from VY, and the results stored in VX.
          */
        case 0x0007:
          if (V[(opcode & 0x00F0) >> 4] >= V[(opcode & 0x0F00) >> 8]) V[0xF] = 1;
          else V[0xF] = 0;
          V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
          break;
          /*
            8XYE - SHL VX {, VY}
            Originally: Set VX = VY SHL 1
            Today: Set VX = VX SHL 1
            If the most-significant bit of VX is 1, then VF is set to 1,
            otherwise to 0. Then VX is multiplied by 2.
          */
        case 0x000E:
          V[0xF] = (V[(opcode & 0x0F00) >> 8] >> 7) & 0x01;
          V[(opcode & 0x0F00) >> 8] <<= 1;
          break;
      }
      break;
      /*
        9XY0 - SNE VX, VY
        Skip next instruction if VX != VY.
        The values of VX and VY are compared, and if they are not equal, the
        program counter is increased by 2.
      */
    case 0x9000:
      if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4]) pc += 2;
      break;
      /*
        ANNN - LD I, addr
        Set I = NNN.
        The value of register I is set to nnn.
      */
    case 0xA000:
      I = opcode & 0x0FFF;
      break;
      /*
        BNNN - JP V0, addr
        Jump to location NNN + V0.
        The program counter is set to NNN plus the value of V0.
      */
    case 0xB000:
      pc = (opcode & 0x0FFF) + V[0];
      break;
      /*
        CXNN - RND VX, NN
        Set VX = random byte AND NN.
        The interpreter generates a random number from 0 to 255, which
        is then ANDed with the value NN. The results are stored in
        VX.
      */
    case 0xC000:
      V[(opcode & 0x0F00) >> 8] = (rand() % 0x100) & (opcode & 0x00FF);
      break;
      /*
        DXYN - DRW VX, VY, N
        Display N-byte sprite starting at memory location I at (VX, VY), set VF = collision.
        The interpreter reads n bytes from memory, starting at the address
        stored in I. These bytes are then displayed as sprites on screen
        at coordinates (VX, VY). Sprites are XORed onto the existing
        screen. If this causes any pixels to be erased, VF is set to 1,
        otherwise it is set to 0. If the sprite is positioned so part of
        it is outside the coordinates of the display, it wraps around to
        the opposite side of the screen. See instruction 8XY3 for more
        information on XOR, and section 2.4, Display, for more information
        on the Chip-8 screen and sprites.
      */
    case 0xD000: //{
      drawSprite(((opcode & 0x0F00)>>8), ((opcode & 0x00F0)>>4), (opcode & 0x000F));
      CHIP8_drawFlag = true;
      break;
    case 0xE000:
      switch (opcode & 0x00FF) {
          /*
            Ex9E - SKP VX
            Skip next instruction if key with the value of VX is pressed.
            Checks the keyboard, and if the key corresponding to the value of VX
            is currently in the down position, PC is increased by 2.
          */
        case 0x009E:
          if (key[V[(opcode & 0x0F00) >> 8]] != 0) pc += 2;
          break;
          /*
            ExA1 - SKNP VX
            Skip next instruction if key with the value of VX is not pressed.
            Checks the keyboard, and if the key corresponding to the value of VX
            is currently in the up position, PC is increased by 2.
          */
        case 0x00A1:
          if (key[V[(opcode & 0x0F00) >> 8]] == 0) pc += 2;
          break;
      }
      break;
    case 0xF000:
      switch (opcode & 0x00FF) {
          /*
            FX07 - LD VX, DT
            Set VX = delay timer value.
            The value of DT is placed into VX.
          */
        case 0x0007:
          V[(opcode & 0x0F00) >> 8] = delay_timer;
          break;
          /*
            FX0A - LD VX, N
            Wait for a key press, store the value of the key in VX.
            All execution stops until a key is pressed, then the value of that key is stored in VX.
          */
        case 0x000A:
            pc -= 2;
            for (curr_key = 0; curr_key < 16; curr_key++) {
              if (key[curr_key] != 0) {
                V[(opcode & 0x0F00) >> 8] = curr_key;
                keyPress = true;
                pc += 2;
                break;
              }
            }
          break;
          /*
            FX15 - LD DT, VX
            Set delay timer = VX.
            DT is set equal to the value of VX.
          */
        case 0x0015:
          delay_timer = V[(opcode & 0x0F00) >> 8];
          break;
          /*
            FX18 - LD ST, VX
            Set sound timer = VX.
            ST is set equal to the value of VX.
          */
        case 0x0018:
          sound_timer = V[(opcode & 0x0F00) >> 8];
          break;
          /*
            FX1E - ADD I, VX
            Set I = I + VX.
            The values of I and VX are added, and the results are stored in I.
          */
        case 0x001E:;
          if (I + V[(opcode & 0x0F00) >> 8] > 0xFFF) V[0xF] = 1;
          else V[0xF] = 0;
          I += V[(opcode & 0x0F00) >> 8];
          break;
          /*
            FX29 - LD F, VX
            Set I = location of sprite for digit VX.
            The value of I is set to the location for the hexadecimal sprite
            corresponding to the value of VX.
          */
        case 0x0029:
          I = V[(opcode & 0x0F00) >> 8] * 0x5;
          break;
          /*
            FX30 - LD HF, VX
            Set I = location of sprite for large digit VX.
            The value of I is set to the location for the hexadecimal sprite
            corresponding to the value of VX.
          */
        case 0x0030:
          I = V[((opcode & 0x0F00)>>8)] * 10 + 80;
          break;
          /*
            FX33 - LD B, VX
            Store BCD representation of VX in memory locations I, I+1, and I+2.
            The interpreter takes the decimal value of VX, and places the
            hundreds digit in memory at location in I, the tens digit at
            location I+1, and the ones digit at location I+2.
          */
        case 0x0033:;
          int n;
          n = V[(opcode & 0x0F00) >> 8];
          memory[I]     = (n - (n % 100)) / 100;
          n -= memory[I] * 100;
          memory[I + 1] = (n - (n % 10)) / 10;
          n -= memory[I+1] * 10;
          memory[I + 2] = n;
          break;
          /*
            FX55 - LD [I], VX
            Store registers V0 through VX in memory starting at location I.
            The interpreter copies the values of registers V0 through VX
            into memory, starting at the address in I.
          */
        case 0x0055:
          for (int n=0; n <= ((opcode & 0x0F00)>>8); n++)
              memory[I+n] = V[n];
          break;
          /*
            FX65 - LD VX, [I]
            Read registers V0 through VX from memory starting at location I.
            The interpreter reads values from memory starting at location
            I into registers V0 through VX.
          */
        case 0x0065:
          for (int n=0; n <= ((opcode & 0x0F00)>>8); n++)
              V[n] = memory[I+n];
          break;
          /*
            FX75 - LD R, VX
            SCHIP-8 Store V0 through VX to HP-48 RPL user flags (X <= 7).
          */
        case 0x0075:
          for (int i=0; i <= ((opcode & 0x0F00)>>8); i++)
              hp48_flags[i] = V[i];
          break;
          /*
            FX85 - LD VX, R
            SCHIP-8 Store HP-48 RPL user flags in V0 through VX (X <= 7).
          */
        case 0x0085:
          for (int i=0; i <= ((opcode & 0x0F00)>>8); i++)
              V[i] = hp48_flags[i];
          break;
      }
      break;
    }
}

void CHIP8_decreaseTimers()
{
    if (delay_timer > 0) --delay_timer; // Update timers
    if (sound_timer != 1) CHIP8_soundFlag = false;
    if (sound_timer > 0) {
    if (sound_timer == 1) CHIP8_soundFlag = true;
    --sound_timer;
    }
}

void CHIP8_setKeys(unsigned int buttonValue){
	if(buttonValue & VPAD_BUTTON_ZL) 	{ key[kconf[8]] = 1; } else { key[kconf[8]] = 0; }
	if(buttonValue & VPAD_BUTTON_ZR) 	{ key[kconf[9]] = 1; } else { key[kconf[9]] = 0; }
	if(buttonValue & VPAD_BUTTON_L) 	{ key[kconf[10]] = 1; } else { key[kconf[10]] = 0; }
	if(buttonValue & VPAD_BUTTON_R) 	{ key[kconf[11]] = 1; } else { key[kconf[11]] = 0; }
	if(buttonValue & VPAD_BUTTON_MINUS) 	{ key[kconf[12]] = 1; } else { key[kconf[12]] = 0; }
    if(buttonValue & VPAD_BUTTON_PLUS) 	{ key[kconf[13]] = 1; } else { key[kconf[13]] = 0; }
    if(buttonValue & VPAD_BUTTON_STICK_L) 	{ key[kconf[14]] = 1; } else { key[kconf[14]] = 0; }
    if(buttonValue & VPAD_BUTTON_STICK_R) 	{ key[kconf[15]] = 1; } else { key[kconf[15]] = 0; }
    if (controlUsed == 0)
    {
        if((buttonValue & VPAD_BUTTON_A) | (buttonValue & VPAD_STICK_R_EMULATION_RIGHT)) 	{ key[kconf[0]] = 1; } else { key[kconf[0]] = 0; }
        if((buttonValue & VPAD_BUTTON_B) | (buttonValue & VPAD_STICK_R_EMULATION_DOWN)) 	{ key[kconf[1]] = 1; } else { key[kconf[1]] = 0; }
        if((buttonValue & VPAD_BUTTON_X) | (buttonValue & VPAD_STICK_R_EMULATION_UP)) 	{ key[kconf[2]] = 1; } else { key[kconf[2]] = 0; }
        if((buttonValue & VPAD_BUTTON_Y) | (buttonValue & VPAD_STICK_R_EMULATION_LEFT)) 	{ key[kconf[3]] = 1; } else { key[kconf[3]] = 0; }
        if((buttonValue & VPAD_BUTTON_LEFT) | (buttonValue & VPAD_STICK_L_EMULATION_LEFT)) 	{ key[kconf[4]] = 1; } else { key[kconf[4]] = 0; }
        if((buttonValue & VPAD_BUTTON_RIGHT) | (buttonValue & VPAD_STICK_L_EMULATION_RIGHT)) 	{ key[kconf[5]] = 1; } else { key[kconf[5]] = 0; }
        if((buttonValue & VPAD_BUTTON_UP) | (buttonValue & VPAD_STICK_L_EMULATION_UP)) 	{ key[kconf[6]] = 1; } else { key[kconf[6]] = 0; }
        if((buttonValue & VPAD_BUTTON_DOWN) | (buttonValue & VPAD_STICK_L_EMULATION_DOWN)) 	{ key[kconf[7]] = 1; } else { key[kconf[7]] = 0; }
    }
    else
    {
        if(buttonValue & VPAD_BUTTON_X) 	{ key[kconf[0]] = 1; } else { key[kconf[0]] = 0; }
        if(buttonValue & VPAD_BUTTON_Y) 	{ key[kconf[1]] = 1; } else { key[kconf[1]] = 0; }
        if(buttonValue & VPAD_BUTTON_A) 	{ key[kconf[2]] = 1; } else { key[kconf[2]] = 0; }
        if(buttonValue & VPAD_BUTTON_B) 	{ key[kconf[3]] = 1; } else { key[kconf[3]] = 0; }
        if(buttonValue & VPAD_BUTTON_UP) 	{ key[kconf[4]] = 1; } else { key[kconf[4]] = 0; }
        if(buttonValue & VPAD_BUTTON_DOWN)	{ key[kconf[5]] = 1; } else { key[kconf[5]] = 0; }
        if(buttonValue & VPAD_BUTTON_RIGHT) 	{ key[kconf[6]] = 1; } else { key[kconf[6]] = 0; }
        if(buttonValue & VPAD_BUTTON_LEFT) 	{ key[kconf[7]] = 1; } else { key[kconf[7]] = 0; }
    }
}
/*
int crand() {
  long seed = OSGetTime(); // the seed value for more randomnes
  seed = (seed * 32719 + 3) % 32749;
  return ((seed % 32719) + 1);
}
*/
