#ifndef CHIPPERINO_SCREEN_H
#define CHIPPERINO_SCREEN_H
#include "utils.hpp"
#include "architecture.hpp"

#define RESET_SCREEN "\033[2J"
#define RESET_CURSOR "\033[H"
#define HIDE_CURSOR "\033[?12l"
#define NEXT_LINE "\033[1E"
#define NEXT_POS "\033[1C"

#define DISPLAY_START "\033[2;2H"

char screen_buffer[chip8_display_height+2][chip8_display_width+3];

void print_border()
{    
    // top border
    screen_buffer[0][0] = '/';
    for (int i = 1; i <= chip8_display_width; ++i)
      screen_buffer[0][i] = '-';
    
    screen_buffer[0][chip8_display_width+1] = '\\';
    // middle
    for (int i = 1; i <= chip8_display_height; ++i)
    {
        printf("|%*s|" NEXT_LINE, chip8_display_width, "");
        screen_buffer[i][0] = '|';
        screen_buffer[i][chip8_display_width+1] = '|';
    }
    // bottom border
    screen_buffer[chip8_display_height+1][0] = '\\';
    for (int i = 1; i <= chip8_display_width; ++i)
      screen_buffer[chip8_display_height+1][i] = '-';
    screen_buffer[chip8_display_height+1][chip8_display_width+1] = '//';
}

void draw_display(chip8_t *c = &chip8)
{
    print_border();

    for (int i = 0; i < chip8_display_height; ++i)
    {
        for (int j = 0; j < chip8_display_width; ++j)
        {
            char glyph = c->display[i][j] ? '*' : ' ';
            screen_buffer[i+1][j+1] = glyph;
        }
    }

    // Put newlines just so we can print everything just in a single printf call
    for (int i = 0; i < chip8_display_height+1; ++i)
      screen_buffer[i][chip8_display_width+2] = '\n';

    // And the null terminator for the right bottom corner
    screen_buffer[chip8_display_height+1][chip8_display_width+2] = '\0';

    // finally, the actual printing to screen
    printf(RESET_CURSOR "%s", screen_buffer);
}

void clear_screen()
{
    printf(RESET_SCREEN);
    printf(RESET_CURSOR);
    fflush(stdout);
}


#endif
