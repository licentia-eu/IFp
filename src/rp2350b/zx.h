#pragma once

#include "zx.pio.h"

#include <cstdio>
#include <limits>

template <typename T>
void printBits(const char *str, T num)
{

    printf("%s ", str);
    const int numBits = std::numeric_limits<T>::digits;

    for (int i = numBits - 1; i >= 0; --i) {
        if ( (i + 1) % 8 == 0)
            putchar(' ');
        if ((num >> i) & 1) {
            putchar('1');
        } else {
            putchar('0');
        }
    }
    uint32_t nr = num;
    printf(" %lX\n", nr);
}

constexpr uint32_t dataBitsMask = (0xff << PIO_BASE);

extern uint8_t *RomPtr;
extern uint16_t RomSize;
void zx_main();
