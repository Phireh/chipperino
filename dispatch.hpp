#ifndef CHIPPERINO_DISPATCH_H
#define CHIPPERINO_DISPATCH_H

#include <memory.h>
#include "architecture.hpp"

void dispatch(chip8_t *c = &chip8)
{
    chip8_instruction_t i = *(chip8_instruction_t *)&c->raw_memory[c->pc];

    // first of all, increment the program counter
    c->pc += 2;
    
    switch (HALF_UPPER_BYTE(i.msb))
    {
    case 0x0: // i: 0x0---
        if (HALF_LOWER_BYTE(i.msb) == 0x0) // i: 0x00--
        {
            if (HALF_LOWER_BYTE(i.lsb) == 0x0) // i: 0x00E0: CLS (clear screen)
            {
                memset(c->display, 0, sizeof(c->display));
            }
            else if (HALF_LOWER_BYTE(i.lsb) == 0xE) // i: 0x00EE: RET
            {
                // TODO: stack pointer underflow detection?
                c->pc = c->stack[--c->sp];
            }
            else
            {
                // unexpected combination
                // TODO: error handling
            }
        }
        else // i: 0x0nnn: SYS addr
        {
            c->pc = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        }
        break;

    case 0x1: // i: 0x1nnn: JMP addr
        c->pc = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        break;

    case 0x2: // i: 0x2nnn: CALL addr
        // TODO: stack pointer overflow detection
        c->stack[c->sp++] = c->pc;
        c->pc = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        break;

    case 0x3: // i: 0x3xkk: SE Vx, byte
    {
        uint8_t reg_contents = c->regs[HALF_LOWER_BYTE(i.msb)];
        if (reg_contents == i.lsb)
            c->pc += sizeof(chip8_instruction_t);
    } break;

    case 0x4: // i: 0x4xkk: SNE Vx, byte
    {
        uint8_t reg_contents = c->regs[HALF_LOWER_BYTE(i.msb)];
        if (reg_contents != i.lsb)
            c->pc += sizeof(chip8_instruction_t);
    } break;
        
    case 0x5: // i: 0x5xy0: SE Vx, Vy
    {
        uint8_t reg1 = c->regs[HALF_LOWER_BYTE(i.msb)];
        uint8_t reg2 = c->regs[HALF_UPPER_BYTE(i.lsb)];
        if (reg1 == reg2)
            c->pc += sizeof(chip8_instruction_t);
    } break;
        
    case 0x6: // i: 0x6xkk: LD Vx, byte        
        c->regs[HALF_LOWER_BYTE(i.msb)] = i.lsb;
        break;

    case 0x7: // i: 0x7xkk: ADD Vx, byte
        c->regs[HALF_LOWER_BYTE(i.msb)] += i.lsb;
        break;

    case 0x8: // i: 0x8---
        switch (HALF_LOWER_BYTE(i.lsb))
        {
        case 0x0: // i: 0x8xy0: LD Vx, Vy
            c->regs[HALF_LOWER_BYTE(i.msb)] = c->regs[HALF_UPPER_BYTE(i.lsb)];
            break;

        case 0x1: // i: 0x8xy1: OR Vx, Vy
            c->regs[HALF_LOWER_BYTE(i.msb)] |= c->regs[HALF_UPPER_BYTE(i.lsb)];
            break;

        case 0x2: // i: 0x8xy2: AND Vx, Vy
            c->regs[HALF_LOWER_BYTE(i.msb)] &= c->regs[HALF_UPPER_BYTE(i.lsb)];
            break;

        case 0x3: // i: 0x8xy3: XOR Vx, Vy
            c->regs[HALF_LOWER_BYTE(i.msb)] ^= c->regs[HALF_UPPER_BYTE(i.lsb)];
            break;

        case 0x4: // i: 0x8xy4: ADD Vx, Vy
        {
            uint16_t res = c->regs[HALF_LOWER_BYTE(i.msb)] + c->regs[HALF_UPPER_BYTE(i.lsb)];
            // Update carry flag
            c->VF = res > 0xFF ? 1 : 0;
            // Only store lower 8b
            c->regs[HALF_LOWER_BYTE(i.msb)] = res & 0xFF;
        } break;

        case 0x5: // i: 0x8xy5: SUB Vx, Vy
        {
            int16_t diff = (int16_t)c->regs[HALF_LOWER_BYTE(i.msb)] - (int16_t)c->regs[HALF_UPPER_BYTE(i.lsb)];
            c->VF = diff > 0 ? 0 : 1;
            c->regs[HALF_LOWER_BYTE(i.msb)] = diff & 0xFF;
        } break;
            

        case 0x6: // i: 0x8xy6: SHR Vx, {,Vy}
            c->VF = c->regs[HALF_LOWER_BYTE(i.msb)] & 1;
            c->regs[HALF_LOWER_BYTE(i.msb)] >>= 1;
            break;

        case 0x7: // i: 0x8xy7: SUBN Vx, Vy
        {
            int16_t diff = (int16_t)c->regs[HALF_LOWER_BYTE(i.lsb)] - (int16_t)c->regs[HALF_UPPER_BYTE(i.msb)];
            c->VF = diff > 0 ? 1 : 0;
            c->regs[HALF_LOWER_BYTE(i.msb)] = diff & 0xFF;
        } break;

        case 0xE: // i: 0x8xyE: SHL Vx, {,Vy}
            c->VF = (c->regs[HALF_LOWER_BYTE(i.msb)] & 0xA000) >> 7;
            c->regs[HALF_LOWER_BYTE(i.msb)] <<= 1;
            break;
        default:
            // TODO: Error handling?
            break;
        }
        break;

    case 0x9: // i: 0x9xy0: SNE Vx, Vy
        if (c->regs[HALF_LOWER_BYTE(i.msb)] != c->regs[HALF_UPPER_BYTE(i.lsb)])
            c->pc += sizeof(chip8_instruction_t);
        break;

    case 0xA: // i: 0xAnnn: LD I, addr
        c->I = (HALF_LOWER_BYTE(i.msb) << 8) | i.lsb;
        break;

    case 0xB: // i: 0xBnnn: JP V0, addr
        c->pc = ((HALF_LOWER_BYTE(i.msb) << 8) | i.lsb) + c->V0;
        break;

    case 0xC: // i: 0xCxkk: RND Vx, byte
    {
        uint32_t r = pcg32_random_r(&c->rng);
        c->regs[HALF_LOWER_BYTE(i.msb)] = r & i.lsb;
    } break;

    case 0xD: // i: 0xDxyn: DRW Vx, Vy, nibble

    {
        // executing an instruction that changes the display
        display_update = true;
        
        uint8_t collision_flag = 0;
        uint8_t target_x = c->regs[HALF_LOWER_BYTE(i.msb)];
        uint8_t x = target_x;
        uint8_t y = c->regs[HALF_UPPER_BYTE(i.lsb)];
        uint8_t nibble = HALF_LOWER_BYTE(i.lsb);
        

        for (int j = 0; j < nibble; ++j, ++y)
        {
            x = target_x;
            // wrap vertically if need be
            y %= chip8_display_height;
            uint8_t sprite = c->raw_memory[c->I+j];
            for (int k = 0; k < 8; ++k, ++x)
            {
                // Wrap horizontally if need be
                x %= chip8_display_width;
                uint8_t prev = c->display[y][x];
                uint8_t next = prev ^ (sprite >> (7-k) & 1);
                c->display[y][x] = next;
                if (prev && !next) // we flipped from 1 to 0
                    collision_flag = 1;
            }
        }
        c->VF = collision_flag;
    } break;
        
    case 0xE: // i: 0xE---
        if (i.lsb == 0x9E) // i: 0xEx9E: SKP Vx
        {
            int8_t keycode = c->regs[HALF_LOWER_BYTE(i.msb)];
            if ((c->input.keys >> keycode) & 1)
                c->pc += 2;
            break;
        }
        else if (i.lsb == 0xA1) // i: 0xExA1: SKNP Vx
        {
            int8_t keycode = c->regs[HALF_LOWER_BYTE(i.msb)];
            if (!((c->input.keys >> keycode) & 1))
                c->pc += 2;
            break;
        }
        else
        {
            // TODO: error handling?
            break;
        }
        break;

    case 0xF: // i: 0xF---
        switch (i.lsb)
        {
        case 0x07: // i: 0xFx07: LD Vx, DT
            c->regs[HALF_LOWER_BYTE(i.msb)] = c->dt;
            break;

        case 0x0A: // i: 0xFx0A: LD Vx, K
        {
            if (c->input.keys)
            {
                for (uint8_t j = 0; j < 16; ++j)
                {
                    uint16_t key = c->input.keys & (1 << j);
                    if (key)
                    {
                        c->regs[HALF_LOWER_BYTE(i.msb)] = j;
                    }
                }
            }
            else
            {
                c->pc -= 2; // return to same instruction as a form of waiting
                return;
            }
            
        } break;

        case 0x15: // i: 0xFx15: LD DT, Vx
            c->dt = c->regs[HALF_LOWER_BYTE(i.msb)];
            break;
            
        case 0x18: // i: 0xFx18: LD ST, Vx
            c->st = c->regs[HALF_LOWER_BYTE(i.msb)];
            break;

        case 0x1E: // i: 0xFx1E: ADD I, 
            c->I += c->regs[HALF_LOWER_BYTE(i.msb)];
            break;

        case 0x29: // i: 0xFx29: LD F, Vx
            c->I = default_font_offset + c->regs[HALF_LOWER_BYTE(i.msb)] * default_letter_size;
            break;

        case 0x33: // i: 0xFx33: LD B, Vx
        {
            uint8_t reg_value = c->regs[HALF_LOWER_BYTE(i.msb)];
            c->raw_memory[c->I] = (reg_value/100) % 10;
            c->raw_memory[c->I+1] = (reg_value/10) % 10;
            c->raw_memory[c->I+2] = reg_value % 10;
        } break;

        case 0x55: // i: 0xFx55
        {
            uint8_t last_idx = HALF_LOWER_BYTE(i.msb);
            for (uint8_t i = 0; i <= last_idx && i < 16; ++i)
            {
                uint8_t value = c->regs[i];
                c->raw_memory[c->I + i] = value;
            }
        } break;

        case 0x65: // i: 0xFx65
        {
            uint8_t last_idx = HALF_LOWER_BYTE(i.msb);
            for (uint8_t i = 0; i <= last_idx && i < 16; ++i)
            {
                uint8_t value = c->raw_memory[c->I + i];
                c->regs[i] = value;
            }
        } break;

        default:
            // TODO: error handling?
            break;        
        }
        break;
    default:
        // TODO: error handling?
        break;        
    }
}

#endif
