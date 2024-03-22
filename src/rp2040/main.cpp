// -----------------------------------------------------------------------------
// ZX Spectrum Interface Pico based on RP2040 PICO30
// -----------------------------------------------------------------------------
// Copyright (C) 2024 BogDan Vatra <bogdan@kde.org>
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
#include <hardware/pio.h>

#ifdef ENABLE_USB_STDIO
#include <tusb.h>
#endif

#include <zx.pio.h>

namespace {
static unsigned char zx_rom[] = {0x21, 0x00, 0x00, 0x7d, 0xfe, 0x24, 0xda, 0x0e, 0x00, 0x7e, 0x23, 0xc3, 0x03, 0x00, 0x21, 0x24, 0x00, 0x7c,
                                 0xfe, 0x40, 0xda, 0x00, 0x00, 0x7e, 0xbd, 0xd2, 0x20, 0x00, 0x23, 0xc3, 0x11, 0x00, 0x76, 0xc3, 0x20, 0x00};

}

int main()
{
#ifdef ENABLE_USB_STDIO
    stdio_init_all();
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }
    puts("\033[2J\033[H");
#endif

    for (int pin = 0; pin < 30; ++pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
    }

    auto setupOutput = [](uint pin, bool value) {
        gpio_init(pin);
        gpio_put(pin, value);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_set_slew_rate(pin, GPIO_SLEW_RATE_FAST);
    };

    setupOutput(O_ROMCS_L, true);
    setupOutput(O_WAIT_L, true);
    setupOutput(O_OE_L, false);

    auto pio = pio0;
    auto sm = pio_claim_unused_sm(pio, true);

    auto offset = pio_add_program(pio, &zx_program);
    pio_sm_config c = zx_program_get_default_config(offset);

    // O_WAIT_L is used by the PIO set the ZX spectum in wait state
    pio_gpio_init(pio, O_WAIT_L);
    sm_config_set_sideset_pins(&c, O_WAIT_L);
    pio_sm_set_consecutive_pindirs(pio, sm, O_WAIT_L, 1, true);

    // I_RD_L is used by the PIO to set the DATA bus pins direction
    sm_config_set_jmp_pin(&c, I_RD_L);

    // B_ADDR_BASE to pin 24 are used by the PIO IN instruction to read the address and data bus
    sm_config_set_in_pins(&c, B_ADDR_BASE);
    sm_config_set_in_shift(&c, false, true, 24);

    // B_DATA_BASE to pin B_DATA_BASE + 8 are used by the PIO OUT instruction to write byte the data bus
    sm_config_set_out_pins(&c, B_DATA_BASE, 8);
    sm_config_set_out_shift(&c, true, true, 24);
    for (int i = B_DATA_BASE; i <= B_DATA_BASE + 8; i++)
        pio_gpio_init(pio, i);

    sm_config_set_clkdiv(&c, 1);

    pio_sm_init(pio, sm, offset, &c);

    pio_sm_drain_tx_fifo(pio, sm);

    pio_sm_set_enabled(pio, sm, true);

    uint8_t rom[0x4000];
    memcpy(rom, zx_rom, sizeof(zx_rom));
    for (int i = sizeof(zx_rom); i < 0x4000; ++i) {
        rom[i] = i;
    }
    constexpr uint16_t MREQ_L_BIT = 1 << I_MREQ_L;
    // constexpr uint16_t RD_L_BIT = 1 << I_RD_L;

    puts("Hello there");

    while (true) {
        auto bus = pio_sm_get_blocking(pio, sm);
        uint16_t addr = bus & 0x0fff;

        uint32_t output = 0;
        //(bus & RD_L_BIT) ? 0x000000 : 0x00ff00;
        if (!(bus & MREQ_L_BIT)) {
            output |= rom[addr];
        }
        printf("bus: %08x addr: %04x, data: %02x mreq:%b, rd:%b \n",
               unsigned(bus),
               unsigned(bus & 0x0fff),
               unsigned(bus >> 16) & 0xff,
               unsigned(bus & (1 << I_MREQ_L)),
               unsigned(bus & (1 << I_RD_L)));

        for (int i = 0; i < 24; ++i) {
            if (!(bus & 1)) {
                if (i < 16) {
                    printf("A%02d ", i);
                } else {
                    printf("D%d ", i - 16);
                }
            }
            bus >>= 1;
        }
        sleep_ms(10);
        pio_sm_put_blocking(pio, sm, 0);
        // uint32_t pins = sio_hw->gpio_in;
        // printf("%032b %i\n", unsigned(pins), !!int(pins & (1 << I_RP_EN_L)));
    }
    return 0;
}
