#ifndef CHIPPERINO_UTILS_H
#define CHIPPERINO_UTILS_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

bool colored_display;

#ifdef __linux__
#include <unistd.h>
#include <termios.h>
#include <errno.h>

bool check_for_terminal()
{
    if (!isatty(fileno(stdout)))
        return false;
    else
        return true;
}
bool check_for_colored_output()
{
    if (!check_for_terminal())
        return false;
    
    char *term = getenv("TERM");
    if (strstr(term, "xterm"))
        return true;
    
    return false;
}
void set_console_raw_mode(bool state, int fd = STDIN_FILENO)
{
    static struct termios original_ts;    
    static bool first_time = true;
    
    struct termios ts;
    
    if (state == true && first_time)
    {
        // record original state of console so we can gracefully restore afterwards
        tcgetattr(fd, &original_ts);
        first_time = false;
    }
    
    if (state == true)
    {
        ts = original_ts;

        ts.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP  | INLCR | IGNCR | ICRNL| IXON);
        // NOTE: we still want OPOST so the terminal interprets '\n' on output
        //ts.c_oflag &= ~OPOST;
        ts.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        ts.c_cflag &= ~(CSIZE | PARENB);
        ts.c_cflag |= CS8;
        
        ts.c_cc[VMIN] = 0;
        ts.c_cc[VTIME] = 0;

        if (tcsetattr(fd, TCSAFLUSH, &ts))
        {
            fprintf(stderr, "Error trying to set console raw mode: %s\n", strerror(errno));
        }
    }
    else
    {
        if (tcsetattr(fd, TCSAFLUSH, &original_ts))
        {
            fprintf(stderr, "Error trying to set console raw mode: %s\n", strerror(errno));
        }
    }
    
    
}
/* NOTE: We're riding on the fact that the terminal becomes non-blocking when VTIME = 0 and VMIN = 0,
   otherwise we would have to use fcntl() to avoid blocking here */
bool read_raw_input(char *c, int n, int fd = STDIN_FILENO)
{
    int ret = read(fd, c, n);
    if (ret == n)
        return true; // read n characters as expected
    else
    {
        if (ret == -1 && errno != EAGAIN)
        {
            // TODO: Error handling
            ;
        }
        return false;
    }
}

#else
#ifdef _WIN32
#include <io.h>
#include <errno.h>
#include <windows.h>
#include <ProcessEnv.h>
bool check_for_terminal()
{
    if (!_isatty(_fileno(stdout)))
        return false;
    else
        return true;
}

bool check_for_colored_output()
{
    if (!check_for_terminal())
        return false;
    
    if (getenv("ANSICON"))
        return true;
    else
        return false;
}

void set_console_raw_mode(bool state, int fd = STD_INPUT_HANDLE)
{
    HANDLE console_handle = GetStdHandle(fd);
    static DWORD original_flags;
    static bool first_time = true;

    if (first_time)
    {
        GetConsoleMode(console_handle, &original_flags);
        first_time = false;
    }

    int ret;
    if (true)
    {
        DWORD flags = original_flags & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        ret = SetConsoleMode(console_handle, flags);
    }
    else
    {
        ret = SetConsoleMode(console_handle, original_flags);
    }
    if (ret)
    {
        fprintf(stderr, "Error trying to set console raw mode\n");
    }
}

bool read_raw_input(char *c, int n, int fd = STD_INPUT_HANDLE)
{
    HANDLE console_handle = GetStdHandle(fd);
    INPUT_RECORD input_buffer;
    DWORD nread;

    // check if there is available input
    while (PeekConsoleInput(console_handle, &input_buffer, 1, &nread) && nread)
    {
        // actually read it
        ReadConsoleInput(console_handle, &input_buffer, 1, &nread);

        KEY_EVENT_RECORD *event = ((KEY_EVENT_RECORD*)&input_buffer.Event);

        // check it is actually a pressed key event
        if (input_buffer.EventType & KEY_EVENT && event->bKeyDown)
        {          
            // move input to our character out variable
            *c = event->uChar.AsciiChar;
            return true;
        }
        // otherwise keep reading to clean the event queue of non-keyboard events
    }

    // no more input in queue 
    return false;

    
    // int ret = _read(fd, c, n);
    // if (ret == n)
    //     return true; // read n characters as expected
    // else
    // {
    //     if (ret == -1 && errno != EAGAIN)
    //     {
    //         // TODO: Error handling
    //         ;
    //     }
    //     return false;
    // }
}
#endif
#endif

#define GREEN_BOLD "\033[1;32m"
#define RED_BOLD   "\033[1;31m"
#define BLUE_BOLD  "\033[1;36m"
#define PINK_BOLD  "\033[1;35m"
#define RESET_TEXT "\033[0m"

#define log_ok(str, ...) do { if (colored_display) printf("[" GREEN_BOLD "OK" RESET_TEXT "] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); else printf("[OK] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while (0)

#define log_fail(str, ...) do { if (colored_display) printf("[" RED_BOLD "FAILED" RESET_TEXT "] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); else printf("[FAILED] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while (0)

#define log_detail(str, ...) do { if (colored_display) printf("[" BLUE_BOLD "DETAIL" RESET_TEXT "] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); else printf("[DETAIL] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while (0)

#define log_summary(str, ...) do { if (colored_display) printf("[" PINK_BOLD "SUMMARY" RESET_TEXT "] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); else printf("[SUMMARY] (%s:%d): " str "\n", __FILE__, __LINE__, ##__VA_ARGS__); } while (0)

#endif
