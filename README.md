# A CHIP8/SCHIP-8 Emulator for WiiU upgraded by brienj of XHP Creations

Added GX2 Graphics, Sound Support, and Super CHIP-8 Support

Originally Ported by rw-r-r_0644 using original emulator from here: http://www.multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/

rw-r-r_0644 version located here:  https://github.com/rw-r-r-0644/CHIP8-Emulator-WiiU

CHIP-8/SCHIP-8 GX2 Emulator by brienj

I updated the CHIP-8 Emulator made by rw-r-r_0644 to use GX2 graphics, include the buzzer, and support the Super CHIP-8 interpreter.

Setup:
Put the CHIP8.elf file in the sd:/wiiu/apps/CHIP8/ folder of your SD card.  Put all of your CHIP-8/SCHIP-8 roms in the sd:/roms/CHIP8/ folder of the SD card with the .ch8 extension.  Sub-folders are acceptable, but not needed.

Instructions:
Load a ROM and press the Play button.  If you select the play button with a WiiMote, that WiiMote will be used as the controller, with sideways orientation.  The A and B buttons will be swapped with the 1 and 2 buttons, and the X and Y buttons will be swapped with the A and B buttons, if you play with a WiiMote.  Once a game is started the Play button becomes a Pause button.  Selecting the Config button will open up the controller configuration.  Select a controller button on the screen and then select the button number you wish to assign it to.  Pressing the Save Config button on the configuration screen will save a 16 byte config file in the same directory with the ROM.  Each config file is for each individual ROM, so each ROM can each have a different configuration.  The Left-Stick on the gamepad emulates the D-PAD, and the Right-Stick on the gamepad emulates the A, B, X, and Y buttons.  This allows you to easily play two player pong on the gamepad using the sticks.

Planned to be added:
CHIP-8E, CHIP-8X instructions
Display folder name on selection buttons

Credits:
-rw-r-r_0644 for original port
-dimok for all of his HBL code
