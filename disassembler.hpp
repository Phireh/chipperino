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
        uint8_t raw_memory[memory_size];
        chip8_memory_t memory;
    };
} chip8_t;

// Global var representing the CHIP8 currently being emulated
chip8_t chip8;

uint16_t memory_offset(chip8_instruction_t *i)
{
    return (i - &chip8.memory.as_words[0]) * sizeof(chip8_instruction_t);
}

/** Disassembler info **/
typedef struct {
    std::string mnemonic;
    int nparams;
    std::string summary;
    uint16_t params[3];
} instruction_info_t;

std::map<std::string, instruction_info_t> instruction_table;

// C++ does not provide a const time std::map so we have to initialize it at program startup
void fill_instruction_info()
{
    instruction_table["00E0"] = { "CLS", 0, "clear the display", {}};
    instruction_table["00EE"] = { "RET", 0, "return from a subroutine", {}};
    instruction_table["0nnn"] = { "SYS addr", 1, "jump to machine code routine at nnn", {}};
    instruction_table["1nnn"] = { "JP addr", 1, "jump to location nnn", {}};    
    instruction_table["2nnn"] = { "CALL addr", 1, "call subroutine at location nnn", {}};
    instruction_table["3xkk"] = { "SE Vx, byte", 2, "skip next instruction if Vx = kk", {}};
    instruction_table["4xkk"] = { "SNE Vx, byte", 2, "skip next instruction if Vx != kk", {}};
    instruction_table["5xy0"] = { "SE Vx, Vy", 2, "skip next instruction if Vx == Vy" , {}};
    instruction_table["6xkk"] = { "LD Vx, byte", 2, "put value kk into register Vx" , {}};
    instruction_table["7xkk"] = { "ADD Vx, byte", 2, "adds value kk to register Vx and stores result in Vx" , {}};
    instruction_table["8xy0"] = { "LD Vx, Vy", 2, "stores the value of register Vy in register Vx" , {}};
    instruction_table["8xy1"] = { "OR Vx, Vy", 2, "bitwise OR of Vx and Vy storing result in Vx" , {}};
    instruction_table["8xy2"] = { "AND Vx, Vy", 2, "bitwise AND of Vx and Vy storing result in Vx" , {}};
    instruction_table["8xy3"] = { "XOR Vx, Vy", 2, "bitwise XOR of Vx and Vy storing result in Vx", {}};
    instruction_table["8xy4"] = { "ADD Vx, Vy", 2, "adds values inside Vx and Vy storing result in Vx", {}};
    instruction_table["8xy5"] = { "SUB Vx, Vy", 2, "subtracts value of Vy from Vx storing result in Vx", {}};
    instruction_table["8xy6"] = { "SHR Vx {, Vy}", 2, "shift right the contents of Vx", {}};
    instruction_table["8xy7"] = { "SUBN Vx {, Vy}", 2, "subtracts value of Vy from Vx storing result in Vx", {}};
    instruction_table["8xyE"] = { "SHL Vx {, Vy}", 2, "shift right the contents of Vx", {}};
    instruction_table["9xy0"] = { "SNE Vx, Vy", 2, "skip next instruction if Vx != Vy", {}};
    instruction_table["Annn"] = { "LD I, addr", 1, "set I = nnn", {}};
    instruction_table["Bnnn"] = { "JP V0, addr", 1, "jump to location nnn + V0", {}};
    instruction_table["Cxkk"] = { "RND Vx, byte", 2, "set Vx = random byte AND kk", {}};
    instruction_table["Dxyn"] = { "DRW Vx, Vy, nibble", 3, "display n-byte sprite starting at I at (Vx, Vy)", {}};
    instruction_table["Ex9E"] = { "SKP Vx", 1, "skip next instruction if key with value of Vx is pressed", {}};
    instruction_table["ExA1"] = { "SKNP Vx", 1, "skip next instruction if key with value of Vx is not pressed", {}};
    instruction_table["Fx07"] = { "LD Vx, DT", 1, "the value of DT is placed at Vx", {}};
    instruction_table["Fx0A"] = { "LD Vx, K", 1, "wait for keypress storing the value at Vx", {}};
    instruction_table["Fx15"] = { "LD DT, Vx", 1, "the value of Vx is placed at DT", {}};
    instruction_table["Fx18"] = { "LD ST, Vx", 1, "the value of Vx is placed at ST", {}};
    instruction_table["Fx1E"] = { "ADD I, Vx", 1, "the values of Vx and I are added and stored at I", {}};
    instruction_table["Fx29"] = { "LD F, Vx", 1, "the value of I is set to the location of the sprite at Vx", {}};
    instruction_table["Fx33"] = { "LD B, Vx", 1, "store the hundreds digit of Vx at I, tenths at I+1, units at I+2", {}};
    instruction_table["Fx55"] = { "LD [I], Vx", 1, "store registers V0 through Vx in memory at address I", {}};
    instruction_table["Fx65"] = { "LD [I], Vx", 1, "read memory at address I to registers from V0 to Vk", {}};
    instruction_table["error"] = { "error", 0, "unknown function", {}};
}

#define HALF_UPPER_BYTE(b) (b >> 4)
#define HALF_LOWER_BYTE(b) (0x0F & b)


instruction_info_t disassemble(chip8_instruction_t i)
{
    // Start looking at only the first half of the msb
    instruction_info_t info;
    switch (HALF_UPPER_BYTE(i.msb))
    {
    case 0x0: // i: 0x0---
        if (HALF_LOWER_BYTE(i.msb) == 0x0) // i: 0x00--
        {
            if (HALF_LOWER_BYTE(i.lsb) == 0x0)
                return instruction_table["00E0"];
            else if (HALF_LOWER_BYTE(i.lsb) == 0xE)
                return instruction_table["00EE"];
            else
            {
                // unexpected combination
                return instruction_table["error"];
            }
        }
        else // i: 0x0n--
        {
            info = instruction_table["0nnn"];
            info.params[0] = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
            return info;
        }
        break;

    case 0x1: // i: 0x1---
        info = instruction_table["1nnn"];
        info.params[0] = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        return info;
        break;

    case 0x2: // i: 0x2---
        info = instruction_table["2nnn"];
        info.params[0] = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        return info;
        break;

    case 0x3: // i: 0x3---
        info = instruction_table["3xkk"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = i.lsb;
        return info;
        break;

    case 0x4: // i: 0x4---
        info = instruction_table["4xkk"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = i.lsb;
        return info;
        break;
        
    case 0x5: // i: 0x5---
        info = instruction_table["5xy0"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = HALF_UPPER_BYTE(i.lsb);
        return info;
        break;
        
    case 0x6: // i: 0x6---
        info = instruction_table["6xkk"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = i.lsb;
        return info;        
        break;

    case 0x7: // i: 0x7---
        info = instruction_table["7xkk"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = i.lsb;
        return info;                
        break;

    case 0x8: // i: 0x8---
        switch (HALF_LOWER_BYTE(i.lsb))
        {
        case 0x0: // i: 0x8--0
            info = instruction_table["8xy0"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                
            break;

        case 0x1: // i: 0x8--1
            info = instruction_table["8xy1"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                            
            break;

        case 0x2: // i: 0x8--2
            info = instruction_table["8xy2"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                
            break;

        case 0x3: // i: 0x8--3
            info = instruction_table["8xy3"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                            
            break;

        case 0x4: // i: 0x8--4
            info = instruction_table["8xy4"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                                        
            break;

        case 0x5: // i: 0x8--5
            info = instruction_table["8xy5"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                                        
            break;

        case 0x6: // i: 0x8--6
            info = instruction_table["8xy6"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                                        
            break;

        case 0x7: // i: 0x8--7
            info = instruction_table["8xy7"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                                                    
            break;

        case 0xE: // i: 0x8--E
            info =  instruction_table["8xyE"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            info.params[1] = HALF_UPPER_BYTE(i.lsb);
            return info;                                                                
            break;
        default:
            return instruction_table["error"];
            break;
        }
        break;

    case 0x9: // i: 0x9---
        info = instruction_table["9xy0"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = HALF_UPPER_BYTE(i.lsb);
        return info;                                                                
        break;

    case 0xA: // i: 0xA---
        info = instruction_table["Annn"];
        info.params[0] = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        return info;        
        break;

    case 0xB: // i: 0xB---
        info = instruction_table["Bnnn"];
        info.params[0] = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        return info;                
        break;

    case 0xC: // i: 0xC---
        info = instruction_table["Cxkk"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = i.lsb;
        return info;
        break;

    case 0xD: // i: 0xD---
        info = instruction_table["Dxyn"];
        info.params[0] = HALF_LOWER_BYTE(i.msb);
        info.params[1] = HALF_UPPER_BYTE(i.lsb);
        info.params[1] = HALF_LOWER_BYTE(i.lsb);
        return info;
        break;
        
    case 0xE: // i: 0xE---
        if (i.lsb == 0x9E) // i: 0xE-9E
        {
            info = instruction_table["Ex9E"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;
        }
        else if (i.lsb == 0xA1) // i: 0xE-A1
        {
            info = instruction_table["ExA1"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;
        }
        else
        {
            return instruction_table["error"];
        }
        break;

    case 0xF: // i: 0xF---
        switch (i.lsb)
        {
        case 0x07: // i: 0xF-07
            info = instruction_table["Fx07"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;            
            break;

        case 0x0A: // i: 0xF-0A
            info = instruction_table["Fx0A"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;
            break;

        case 0x15: // i: 0xF-15
            info = instruction_table["Fx15"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;            
            break;
            
        case 0x18: // i: 0xF-18
            info = instruction_table["Fx18"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;                        
            break;

        case 0x1E: // i: 0xF-1E
            info = instruction_table["Fx1E"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;                        
            break;

        case 0x29: // i: 0xF-29
            info = instruction_table["Fx29"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;                        
            break;

        case 0x33: // i: 0xF-33
            info = instruction_table["Fx33"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;                        
            break;

        case 0x55: // i: 0xF-55
            info = instruction_table["Fx55"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;                        
            break;

        case 0x65: // i: 0xF-65
            info = instruction_table["Fx65"];
            info.params[0] = HALF_LOWER_BYTE(i.msb);
            return info;                                    
            break;

        default:
            return instruction_table["error"];
            break;        
        }
        break;
    default:
        return instruction_table["error"];
        break;        
    }
    return instruction_table["error"];
}
