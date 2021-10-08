/* Typical CHIP8 emulator bindings. You may edit these and recompile to change them. 
   These are 1 Byte ASCII characters at the moment */

#define CHIP8_KEY_0 'X'
#define CHIP8_KEY_1 '1'
#define CHIP8_KEY_2 '2'
#define CHIP8_KEY_3 '3'
#define CHIP8_KEY_4 'Q'
#define CHIP8_KEY_5 'W'
#define CHIP8_KEY_6 'E'
#define CHIP8_KEY_7 'A'
#define CHIP8_KEY_8 'S'
#define CHIP8_KEY_9 'D'
#define CHIP8_KEY_A 'Z'
#define CHIP8_KEY_B 'C'
#define CHIP8_KEY_C '4'
#define CHIP8_KEY_D 'R'
#define CHIP8_KEY_E 'F'
#define CHIP8_KEY_F 'V'

/* The "end simulation" key. Since we read 1-byte-at-a-time at the moment,
   ESC (27) is not a great choice, since many keys get translated into ESC+more bytes */
#define CHIP8_KEY_END 'K'
