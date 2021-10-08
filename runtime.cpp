#include "utils.hpp"
#include "architecture.hpp"
#include "screen.hpp"
#include "keybindings.hpp"
#include "dispatch.hpp"
#include <chrono>
#include <ctype.h>

typedef std::chrono::high_resolution_clock Clock;
// The time we wait before peeking into player input
auto input_polling_period = std::chrono::duration<double, std::milli>(1000/60.0);
// The Chip8 DT register has a 60 Hz update freq
auto dt_decrement_period = std::chrono::duration<double, std::milli>(1000/60.0);

void execute(char *filename)
{
    // ensure we are in an interactive enviroment
    check_for_terminal();
    
    FILE *file_handle = fopen(filename, "rb");
    chip8_instruction_t *p = &chip8.memory.as_words[program_offset/sizeof(chip8_instruction_t)];
    chip8_instruction_t i;

    // Read instructions one-by-one
    while (p < ((chip8_instruction_t *) &chip8.memory + 4096) && fread(&i, sizeof(chip8_instruction_t), 1, file_handle))
    {
        *p = i;            // Save written instruction to memory 
        ++p;               // advance pointer
        program_size += 2; // and count bytes read
    }
    fclose(file_handle);

    // set terminal to raw mode so we can have a pretty display
    set_console_raw_mode(true);

    /* End of misc. preparations */
    
    /** vvv Proper runtime section vvv **/

    auto input_timestamp = Clock::now();
    auto dt_timestamp = Clock::now();
    // continue the VM until we are outside the program's memory region
    while(chip8.pc < program_offset + program_size)
    {
        // Sleep for 2 usecs roughly translates into 500 MHz
        usleep(2);

        // update the timers for this clock cycle
        auto now = Clock::now();
        auto input_dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - input_timestamp);
        auto dt_dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - dt_timestamp);
       
        // decrement DT register every 1/60 s
        if (dt_dt > dt_decrement_period)
        {
            dt_timestamp = now;
            if (chip8.dt > 0)
                --chip8.dt;
        }
        
        // Only read input if enough time has passed
        if (input_dt > input_polling_period)
        {

            input_timestamp = now;
            /* Input handling */
            char c = 0;
            chip8.input.keys = 0; // reset key state


            while (read_raw_input(&c, 1)) // consume all pending keypresses
            {
                switch (toupper(c))
                {
                case CHIP8_KEY_END:
                    goto exit_simulation;
                    break;
                    
                case CHIP8_KEY_0:
                    chip8.input.key_0 = true;
                    break;
                case CHIP8_KEY_1:
                    chip8.input.key_1 = true;
                    break;

                case CHIP8_KEY_2:
                    chip8.input.key_2 = true;
                    break;

                case CHIP8_KEY_3:
                    chip8.input.key_3 = true;
                    break;

                case CHIP8_KEY_4:
                    chip8.input.key_4 = true;
                    break;

                case CHIP8_KEY_5:
                    chip8.input.key_5 = true;
                    break;

                case CHIP8_KEY_6:
                    chip8.input.key_6 = true;
                    break;

                case CHIP8_KEY_7:
                    chip8.input.key_7 = true;
                    break;

                case CHIP8_KEY_8:
                    chip8.input.key_8 = true;
                    break;

                case CHIP8_KEY_9:
                    chip8.input.key_9 = true;
                    break;

                case CHIP8_KEY_A:
                    chip8.input.key_a = true;
                    break;

                case CHIP8_KEY_B:
                    chip8.input.key_b = true;
                    break;

                case CHIP8_KEY_C:
                    chip8.input.key_c = true;
                    break;

                case CHIP8_KEY_D:
                    chip8.input.key_d = true;
                    break;

                case CHIP8_KEY_E:
                    chip8.input.key_e = true;
                    break;

                case CHIP8_KEY_F:
                    chip8.input.key_f = true;
                    break;

                default:
                    break;
                }
            }
        }
        

        // execute next instruction
        
        dispatch();
        
        
        
        if (display_update)
        {
            clear_screen();
            draw_display();
            display_update = false;
            fflush(stdout);        
        }
    }

exit_simulation:
    // restore console normal config
    set_console_raw_mode(false);
    // clearing screen on normal mode should draw the console prompt
    clear_screen();
}
