#include "utils.h"

#include <cstdio>

void error(std::string_view message)
{
#ifdef ENABLE_USB_STDIO
    for (int i = 10; i < 25; ++i)
        printf("\033[%i;1H\033[K", i + 1);

    // set cursor to line 10
    puts("\033[10;1H");

    printf("\033[31mError: %s\033[37m\n", message.data());
#endif
}

void notice(std::string_view message)
{
#ifdef ENABLE_USB_STDIO
    for (int i = 10; i < 25; ++i)
        printf("\033[%i;1H\033[K", i + 1);

    // set cursor to line 10
    puts("\033[10;1H");

    printf("\033[35mNotice: %s\033[37m\n", message.data());
#endif
}
