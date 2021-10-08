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

void print_border()
{    
    printf(RESET_CURSOR);
    // top border
    printf("/");
    printf("%s", std::string(chip8_display_width, '-').c_str());
    printf("\\" NEXT_LINE);    
    // middle
    for (int i = 0; i < chip8_display_height; ++i)
    {
        printf("|%*s|" NEXT_LINE, chip8_display_width, "");
    }
    // bottom border
    printf("\\");
    printf("%s", std::string(chip8_display_width, '-').c_str());
    printf("/");
}

void draw_display(chip8_t *c = &chip8)
{
    print_border();
    printf(DISPLAY_START);    

    for (int i = 0; i < chip8_display_height; ++i)
    {
        for (int j = 0; j < chip8_display_width; ++j)
        {
            char glyph = c->display[i][j] ? '*' : ' ';
            printf("%c", glyph);
        }
        printf(NEXT_LINE);
        printf(NEXT_POS);
    }
}

void clear_screen()
{
    printf(RESET_SCREEN);
    printf(RESET_CURSOR);
    fflush(stdout);
}


#endif
