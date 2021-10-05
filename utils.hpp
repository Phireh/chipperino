#ifndef CHIPPERINO_UTILS_H
#define CHIPPERINO_UTILS_H
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

bool colored_display;

#ifdef __linux__
bool check_for_colored_output()
{
    if (!isatty(fileno(stdout)))
        return false;
    
    char *term = getenv("TERM");
    if (strstr(term, "xterm"))
        return true;
    
    return false;
}
#else
#ifdef _WIN32
bool check_for_colored_output()
{
    if (!_isatty( _fileno(stdout)))
        return false;
    
    if (getenv("ANSICON"))
        return true;
    else
        return false;
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
