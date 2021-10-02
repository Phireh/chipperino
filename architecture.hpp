#ifndef CHIPPERINO_ARCHITECTURE_H
#define CHIPPERINO_ARCHITECTURE_H

#include <stdio.h>
#include <stdint.h>
#include <map>
#include <string>

/** Memory layout **/

// Max. memory size in B that the CHIP8 can address with 16b (0xFFFF, 4KB)
const uint16_t memory_size = 4096;

// The first 512 B are reserved memory
const uint16_t program_offset = 512;

// Counter of how many B we read from our ROM
uint16_t program_size = 0;

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
    /* Secondary memory region */
    union {
        uint8_t raw_memory[memory_size] = {};
        chip8_memory_t memory;
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

    /* Miscelaneous */
    pcg32_random_t rng = { 0x853c49e6748fea9bULL, 0xda3e39cb94b95bdbULL };
    
} chip8_t;

// Global var representing the CHIP8 currently being emulated
chip8_t chip8;

uint16_t memory_offset(chip8_instruction_t *i)
{
    return (i - &chip8.memory.as_words[0]) * sizeof(chip8_instruction_t);
}

#endif
