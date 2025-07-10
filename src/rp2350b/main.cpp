
// -----------------------------------------------------------------------------
// This file is part of IfP "Interface Pico"
// Copyright (C) 2025 BogDan Vatra <bogdan@kde.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
// -----------------------------------------------------------------------------

#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <pico/multicore.h>

#include <tusb.h>

#include "zx.h"

extern unsigned char __48_rom[];
extern unsigned char testrom_bin[];
using namespace std;

//#define NO_PIO
// #define PIO_DEBUG
#ifdef PIO_DEBUG
void wait_callback(uint gpio, uint32_t events)
{
    // printf("GPIO %d interrupt, event mask: 0x%02lx\n", gpio, events);
}
#endif

int main()
{
#ifdef PIO_DEBUG
    stdio_init_all();
    while (!tud_cdc_connected())
        sleep_ms(100);

    puts("\033[2J\033[HHello there !\n");

    RomPtr = testrom_bin;
#ifndef NO_PIO
    auto setInGpio = [](int pos) {
        gpio_init(pos);
        gpio_set_dir(pos, GPIO_IN);
        gpio_pull_down(pos);

    };
    auto initInGPIOs = [&](int start, int stop) {
        for (int i = start; i < stop; ++i)
            setInGpio(i);
    };

    auto setOutGpio = [](int pos) {
        gpio_init(pos);
        gpio_set_dir(pos, GPIO_OUT);
        gpio_put(pos, false);
    };
    auto initOutGPIOs = [&](int start, int stop) {
        for (int i = start; i < stop; ++i)
            setOutGpio(i);
    };
    initInGPIOs(0, 8);

    gpio_set_dir_out_masked(0xff);
    gpio_put_all(0);
    gpio_set_dir_in_masked(0xff);

    setInGpio(O_WAIT_L);
    initOutGPIOs(12, 16);

    gpio_put(I_IORQ_L, true);
    gpio_put(I_WR_L, true);
    gpio_put(I_MREQ_L, true);
    gpio_put(I_RD_L, true);
#else
    gpio_init(26);
    gpio_set_dir(26, GPIO_OUT);
    gpio_put(26, true);
#endif

#endif
#ifndef NO_PIO
    multicore_reset_core1();
    multicore_launch_core1(&zx_main);
#ifndef PIO_DEBUG
    stdio_init_all();
#endif

    // tell core1 that stdio is initialized
    multicore_fifo_push_blocking(0);

#ifdef PIO_DEBUG
    gpio_set_irq_enabled_with_callback(O_WAIT_L, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &wait_callback);

    enum Tests : uint8_t {
        RD = 0x01,
        WR = 0x02,
        MREQ = 0x04,
        IORQ = 0x08,
    };

    // auto tests = Tests{Tests::WR | Tests::IORQ};
    // auto tests = Tests{Tests::WR | Tests::MREQ}; // nothing
    // auto tests = Tests{Tests::RD | Tests::IORQ};
    auto tests = Tests{Tests::RD | Tests::MREQ};
#else
    // uint8_t maxLine = 160;
#endif
#endif

    while (true) {
#ifdef NO_PIO
        uint32_t hi, lo;
        pico_default_asm_volatile ("mrrc p0, #0, %0, %1, c8" : "=r" (lo), "=r" (hi));
        uint16_t addr = hi >> 16;
        if (addr)
            gpio_put(26, false);

        uint16_t data = lo >> 16;

        if ((data & 0b0011) & RomPtr[addr] && addr < 0xc000) {
            lo += 1;
        }
        pico_default_asm_volatile ("mcrr p0, #0, %0, %1, c0" : : "r" (lo), "r" (hi));

        gpio_put(26, true);
        continue;
#else
#ifdef PIO_DEBUG
        if (tests & Tests::IORQ) {
            gpio_put(I_IORQ_L, false);
        } else if (tests & Tests::MREQ) {
            gpio_put(I_MREQ_L, false);
        }

        if (tests & Tests::WR) {
            gpio_set_dir_out_masked(0xff);
            gpio_put_masked(0xff, 0xbd);
            gpio_put(I_WR_L, false);
        } else if (tests & Tests::RD) {
            gpio_put(I_RD_L, false);
        }

        sleep_us(1);
        // printBits("MREQ & RD  low", uint16_t(gpio_get_all()));
        // sleep_ms(250);

#ifndef PIO
        // printf("RD : %d, MREQ: %d\n", (int)gpio_get(PIO_BASE + I_RD_L), (int)gpio_get(PIO_BASE + I_MREQ_L));
        // gpio_put(PIO_BASE + O_WAIT_L, gpio_get(PIO_BASE + I_RD_L));
#endif

        gpio_put(I_MREQ_L, true);
        gpio_put(I_IORQ_L, true);
        gpio_put(I_WR_L, true);
        gpio_put(I_RD_L, true);
        // gpio_set_dir_in_masked(0xff);
        // printBits("MREQ & RD high",uint16_t(gpio_get_all()));

#ifndef PIO
        // printf("RD : %d, MREQ: %d\n", (int)gpio_get(PIO_BASE + I_RD_L), (int)gpio_get(PIO_BASE + I_MREQ_L));
        // gpio_put(PIO_BASE + O_WAIT_L, gpio_get(PIO_BASE + I_RD_L));
#endif
#else
        sleep_ms(1000);
        // putchar('.');
        // if (!--maxLine) {
        //     maxLine = 160;
        //     putchar('\n');
        // }
#endif
#endif // #ifdef NO_PIO
    }
    return 0;
}
