#include "architecture.hpp"
#include "utils.hpp"
#include "dispatch.hpp"
#include "screen.hpp"

typedef bool test_f(void);
extern test_f *tests[];

#define TEST(name) bool _##name##_test(void)

struct record_test {
    // HACK: this is a constructor purely to get around the compiler complaining about "tests" not being a type
    record_test(test_f fn, int n)
    {
        tests[n] = fn;
    }
};

#define RECORD_TEST(fn) record_test _aux_##fn(_##fn##_test, __COUNTER__)

TEST(clear_screen)
{
	chip8_t c;
    uint8_t *p = (uint8_t *) c.display;
    // fill the screen with junk
    memset(p, rand(), sizeof(c.display));

    c.memory.as_words[program_offset/sizeof(chip8_instruction_t)] = { 0x00, 0xE0 };
    dispatch(&c);

    for (size_t i = 0; i < sizeof(c.display); ++i)
    {
        if (*p++)
        {
            // non-zero byte
            log_fail("CLS");
            return false;
        }
    }
    log_ok("CLS");
    return true;
}
RECORD_TEST(clear_screen);

TEST(function_call_and_return)
{
    chip8_t c;
    // 0x200: CALL 0x500
    c.memory.as_words[program_offset/sizeof(chip8_instruction_t)] = { 0x25, 0x00 };

    // 0x500: RET
    c.memory.as_words[0x500/sizeof(chip8_instruction_t)] = { 0x00, 0xEE };

    dispatch(&c); // execute CALL

    uint16_t return_address = c.stack[c.sp-1];
    if (return_address != 0x202)
    {
        log_fail("CALL: return address should be 0x%X but is 0x%X", 0x202, return_address);
        return false;
    }

    if (c.pc != 0x500)
    {
        log_fail("CALL: pc should be 0x%X but is 0x%X", 0x500, c.pc);
        return false;
    }

    dispatch(&c); // execute RET
    
    if (c.pc != 0x202)
    {
        log_fail("RET: pc should be 0x%X but is 0x%X", 0x202, c.pc);
        return true;
    }

    if (c.sp)
    {
        log_fail("RET: pc should be %d but is %d", 0, c.sp);
        return true;
    }    

    // if everything is alright
    log_ok("function call and return");
    return true;
}
RECORD_TEST(function_call_and_return);

TEST(jumps)
{
    chip8_t c;
    // 0x200: JMP 0x500
    c.memory.as_words[program_offset/sizeof(chip8_instruction_t)] = { 0x15, 0x00 };
    // 0x500: SE V0, 0xFF
    c.memory.as_words[0x500/sizeof(chip8_instruction_t)] = { 0x30, 0xFF };
    // 0x502: SE V0, 0x00
    c.memory.as_words[0x502/sizeof(chip8_instruction_t)] = { 0x30, 0x00 };
    // 0x506: SNE V1, 0x00
    c.memory.as_words[0x506/sizeof(chip8_instruction_t)] = { 0x41, 0x00 };
    // 0x508: SNE V1, 0xFF
    c.memory.as_words[0x508/sizeof(chip8_instruction_t)] = { 0x41, 0xFF };

    dispatch(&c);

    if (c.pc != 0x500)
    {
        log_fail("JMP: pc should be 0x%x but is 0x%x", 0x500, c.pc);
        return false;
    }

    dispatch(&c);

    if (c.pc != 0x502)
    {
        // V0 should be 0, which != 0xFF, so pc should only increase by 2
        log_fail("SE: pc should be 0x%x but is 0x%x", 0x502, c.pc);
        return false;
    }

    dispatch(&c);

    if (c.pc != 0x506)
    {
        // V0 should be 0, which == 0, so pc should increase by 4
        log_fail("SE: pc should be 0x%x but is 0x%x", 0x506, c.pc);
        return false;
    }

    dispatch(&c);

    if (c.pc != 0x508)
    {
        // V1 should be 0, which == 0, so pc should increase by 2
        log_fail("SNE: pc should be 0x%x but is 0x%x", 0x508, c.pc);
        return false;
    }

    dispatch(&c);

    if (c.pc != 0x50C)
    {
        // V0 should be 0, which != FF, so pc should increase by 4
        log_fail("SNE: pc should be 0x%x but is 0x%x", 0x50C, c.pc);
        return false;
    }

    log_ok("jumps");
    return true;    
}
RECORD_TEST(jumps);

TEST(arithmetic)
{
    chip8_t c;
    // 0x200: ADD v0, 0x5
    c.memory.as_words[program_offset/sizeof(chip8_instruction_t)] = { 0x70, 0x05 };

    dispatch(&c);
    if (c.V0 != 0x5)
    {
        log_fail("ADD: V0 should be 0x%x but is 0x%x", 0x5, c.V0);
        return false;
    }

    // 0x202: ADD v1, 0x7
    c.memory.as_words[0x202/sizeof(chip8_instruction_t)] = { 0x71, 0x07 };

    dispatch(&c);
    if (c.V1 != 0x7)
    {
        log_fail("ADD: V1 should be 0x%x but is 0x%x", 0x7, c.V1);
        return false;
    }

    // 0x204: SUB v0, v1
    c.memory.as_words[0x204/sizeof(chip8_instruction_t)] = { 0x80, 0x15 };
    if (c.VF)
    {
        log_fail("SUB: Vx > Vy, therefore VF should not be set, but is %d", c.VF);
        return false;
    }

    // 0x206: ADD v2, 0xFF
    c.memory.as_words[0x206/sizeof(chip8_instruction_t)] = { 0x72, 0xFF };
    dispatch(&c);

    // 0x208: ADD v3, 0x1
    c.memory.as_words[0x208/sizeof(chip8_instruction_t)] = { 0x73, 0x01 };
    dispatch(&c);

    // 0x20A: ADD v2, v3
    c.memory.as_words[0x20A/sizeof(chip8_instruction_t)] = { 0x82, 0x34 };

    if (!c.VF)
    {
        log_fail("0xFF + 1 should overflow and set VF, but it is 0");
        return false;
    }

    // TODO: maybe test the rest of arithmetic functions
    log_ok("arithmetic");
    return true;
}
RECORD_TEST(arithmetic);

TEST(logic)
{
    chip8_t c;
    // 0x200: LD v0, 0xFF
    c.memory.as_words[program_offset/sizeof(chip8_instruction_t)] = { 0x60, 0xFF };
    dispatch(&c);

    // 0x204: LD v1, 0x0F
    c.memory.as_words[0x202/sizeof(chip8_instruction_t)] = { 0x61, 0x0F };
    dispatch(&c);

    // 0x204: AND v0, v1
    c.memory.as_words[0x204/sizeof(chip8_instruction_t)] = { 0x80, 0x12 };
    dispatch(&c);

    if (c.V0 != 0x0F)
    {
        log_fail("0xFF AND 0x0F should be 0x0F but result is 0x%X", c.V0);
        return false;
    }
    log_ok("logic");
    return true;
}
RECORD_TEST(logic);

TEST(graphics)
{
    chip8_t c;

    // 0x200: LD v0, 0xF
    c.memory.as_words[program_offset/sizeof(chip8_instruction_t)] = { 0x60, 0x0F };

    dispatch(&c);
    
    // 0x202: LD F, v0
    c.memory.as_words[0x202/sizeof(chip8_instruction_t)] = { 0xF0, 0x29 };
    dispatch(&c);

    uint16_t expected_addr = (default_font_offset + default_letter_size * 0xF);
    if (c.I != expected_addr)
    {
        log_fail("LD F: Address of sprite F should be 0x%X but is 0x%X", expected_addr, c.I);
        return false;
    }

    // 0x204: DRW v1,v2, 5
    c.memory.as_words[0x204/sizeof(chip8_instruction_t)] = { 0xD1, 0x25 };
    dispatch(&c);
    
    if (c.VF)
    {
        log_fail("DRW: VF should not be set when drawing on an empty screen but is 0x%X", c.VF);
        return false;
    }

    /* The letter F drawn as a block of bytes */
    char line1[] = {1,1,1,1};
    char line2[] = {1,0,0,0};
    char line3[] = {1,1,1,1};
    char line4[] = {1,0,0,0};
    char line5[] = {1,0,0,0};
    
    if (memcmp(&c.display[0][0], line1, 4) ||
        memcmp(&c.display[1][0], line2, 4) ||
        memcmp(&c.display[2][0], line3, 4) ||
        memcmp(&c.display[3][0], line4, 4) ||
        memcmp(&c.display[4][0], line5, 4))
    {
        log_fail("DRW: display did not draw 'F' correctly");
        return false;
    }
    
    
    log_ok("graphics");
    return true;
};
RECORD_TEST(graphics);

TEST(input)
{
    chip8_t c;

    // 0x200: LD Vx, K
    c.memory.as_words[program_offset/sizeof(chip8_instruction_t)] = { 0xF0, 0x0A };

    dispatch(&c);
    if (c.pc != 0x200)
    {
        log_fail("Fx0A should halt execution until key is pressed");
        return false;
    }

    c.input.key_9 = true; // simulate keypress
    dispatch(&c);
    if (c.pc != 0x202)
    {
        log_fail("Fx0A should resume execution when key is pressed");
        return false;
    }

    if (c.V0 != 0x9)
    {
        log_fail("LD Vx, K should return key 0x9 but returned 0x%X", c.V0);
        return false;
    }

    // 0x204: SKP Vx
    c.memory.as_words[0x202/sizeof(chip8_instruction_t)] = { 0xE0, 0x9E };

    dispatch(&c);
    if (c.pc != 0x206)
    {
        log_fail("SKP V0 (0x9) should skip next instruction since 0x9 is pressed");
        return false;
    }

    log_ok("input");
    return true;
}
RECORD_TEST(input);

/* NOTE: This definition has to be placed after all the test definitions and before main */
test_f *tests[__COUNTER__];
int main()
{
    const int ntests = sizeof(tests)/sizeof(tests[0]);    
    colored_display = check_for_colored_output();

    int successes = 0;
    // Execute all tests
    for (int i = 0; i < ntests; ++i)
    {
        bool result = tests[i]();
        if (result) ++successes;
    }
    log_summary("%d out of %d tests succeeded", successes, ntests);


    // return number of failed tests, or 0 if everything is alright
    return ntests - successes;
}
