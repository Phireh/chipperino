#ifndef CHIPPERINO_ARCHITECTURE_H
#define CHIPPERINO_ARCHITECTURE_H

#include <stdio.h>
#include <stdint.h>
#include <map>
#include <string>
#include <cstddef>

/** Memory layout **/

// Max. memory size in B that the CHIP8 can address with 16b (0xFFFF, 4KB)
const uint16_t memory_size = 4096;

// The first 512 B are reserved memory
const uint16_t program_offset = 512;

// Counter of how many B we read from our ROM
uint16_t program_size = 0;

// Pixel display limits
/* TODO: Maybe turn these into cli arguments?
 Normal chip8 is 64x32
 SuperChip is 128x64
 Megachip is 256x192 + RGB color */
const uint8_t chip8_display_width = 64;
const uint8_t chip8_display_height = 32;

/** PCG32 random number generator **/

typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

#define DEFAULT_FONT_ARR {                      \
        0xF0, 0x90, 0x90, 0x90, 0xF0, /* 0 */   \
        0x20, 0x60, 0x20, 0x20, 0x70, /* 1 */   \
        0xF0, 0x10, 0xF0, 0x80, 0xF0, /* 2 */   \
        0xF0, 0x10, 0xF0, 0x10, 0xF0, /* 3 */   \
        0x90, 0x90, 0xF0, 0x10, 0x10, /* 4 */   \
        0xF0, 0x80, 0xF0, 0x10, 0xF0, /* 5 */   \
        0xF0, 0x80, 0xF0, 0x90, 0xF0, /* 6 */   \
        0xF0, 0x10, 0x20, 0x40, 0x40, /* 7 */   \
        0xF0, 0x90, 0xF0, 0x90, 0xF0, /* 8 */   \
        0xF0, 0x90, 0xF0, 0x10, 0xF0, /* 9 */   \
        0xF0, 0x90, 0xF0, 0x90, 0x90, /* A */   \
        0xE0, 0x90, 0xE0, 0x90, 0xE0, /* B */   \
        0xF0, 0x80, 0x80, 0x80, 0xF0, /* C */   \
        0xE0, 0x90, 0x90, 0x90, 0xE0, /* D */   \
        0xF0, 0x80, 0xF0, 0x80, 0xF0, /* E */   \
        0xF0, 0x80, 0xF0, 0x80, 0x80  /* F */   \
}


const uint16_t default_font_offset = 0x50;
const uint16_t default_font_size = 5*16;
const uint16_t default_letter_size = 5;

/** Actual chip architecture **/

typedef struct {
    union {
        struct {
            uint8_t msb; // most significant byte
            uint8_t lsb; // least significant byte
        };        
        uint8_t bytes[2];        
        uint16_t instruction;
    };
} chip8_instruction_t;

static_assert(sizeof(chip8_instruction_t) == 2);

typedef struct {
    union {
        uint8_t as_bytes[memory_size];
        chip8_instruction_t as_words[memory_size/2];
    };
} chip8_memory_t;

static_assert(sizeof(chip8_memory_t) == 4096);

typedef struct {
    union {
        uint16_t keys; // keys are defined as a bitfield
        struct {
            bool key_f : 1;
            bool key_e : 1;
            bool key_d : 1;
            bool key_c : 1;
            bool key_b : 1;
            bool key_a : 1;
            bool key_9 : 1;
            bool key_8 : 1;
            bool key_7 : 1;
            bool key_6 : 1;
            bool key_5 : 1;
            bool key_4 : 1;
            bool key_3 : 1;
            bool key_2 : 1;
            bool key_1 : 1;
            bool key_0 : 1;
        };
    };
} chip8_input_t;

static_assert(sizeof(chip8_input_t) == 2);

typedef struct {
    /* Secondary memory region */
    union {
        uint8_t raw_memory[memory_size];
        chip8_memory_t memory;
        // HACK: doing this just to have a comfy default initializer
        struct {
            uint8_t preamble[default_font_offset] = {};
            uint8_t font[default_font_size] = DEFAULT_FONT_ARR;
            uint8_t rest[sizeof(chip8_memory_t) - default_font_size - default_font_offset] = {};
        };
    };
    uint16_t stack[16] = {};
    
    /* Registers */
    union {
        struct {
            int8_t V0;
            int8_t V1;
            int8_t V2;
            int8_t V3;
            int8_t V4;
            int8_t V5;
            int8_t V6;
            int8_t V7;
            int8_t V8;
            int8_t V9;
            int8_t VA;
            int8_t VB;
            int8_t VC;
            int8_t VD;
            int8_t VE;
            int8_t VF;
        };
        int8_t regs[16] = {};
    };

    uint16_t I = 0;  // wider I register, usually only rightmost 12b used
    uint16_t pc = 0; // program counter
    uint8_t sp = 0;  // stack pointer
    uint8_t dt = 0;  // delay timer
    uint8_t st = 0;  // sound timer

    /* Display */
    uint8_t display_buffer[chip8_display_height][chip8_display_width] = {};

    /* Input */
    chip8_input_t input;

    /* Miscelaneous */
    pcg32_random_t rng = { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL };
    
} chip8_t;

static_assert(offsetof(chip8_t, stack) == sizeof(chip8_memory_t));


// Global var representing the CHIP8 currently being emulated
chip8_t chip8;

uint16_t memory_offset(chip8_instruction_t *i)
{
    return (i - &chip8.memory.as_words[0]) * sizeof(chip8_instruction_t);
}

// Misc. macros

#define HALF_UPPER_BYTE(b) (b >> 4)
#define HALF_LOWER_BYTE(b) (0x0F & b)

#endif
