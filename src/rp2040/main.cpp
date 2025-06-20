
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

#include <hardware/adc.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/stdlib.h>

#ifdef ENABLE_USB_STDIO
#include <tusb.h>
#endif

#include <zx.pio.h>

namespace {
static unsigned char zx_rom[] = {0x21, 0x00, 0x00, 0x7d, 0xfe, 0x24, 0xd2, 0x0e, 0x00, 0x7e, 0x23, 0xc3, 0x03, 0x00, 0x21, 0x24, 0x00, 0x7c,
                                 0xfe, 0x40, 0xd2, 0x00, 0x00, 0x7e, 0xbd, 0xd2, 0x20, 0x00, 0x23, 0xc3, 0x11, 0x00, 0x76, 0xc3, 0x20, 0x00};
/*************
org 0h
start:
    ld  hl, 0
read_first_part:
    ld a, l
    cp end_of_program
    jp nc, read_second_part
    ld a, (hl)
    inc hl
    jp read_first_part
read_second_part:
    ld  hl, end_of_program
read_loop:
    ld a, h
    cp 40h
    jp nc, start
    ld a, (hl)
    cp l
    jp nc, bad_byte
    inc hl
    jp read_loop
bad_byte:
    halt
    jp  bad_byte
end_of_program:
*************/
constexpr int overclock = 150000;
} // namespace

int main()
{
    set_sys_clock_khz(overclock, true);
#ifdef ENABLE_USB_STDIO
    stdio_init_all();
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }
    puts("\033[2J\033[H");
#endif

    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);

    while (1) {
        // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
        const float conversion_factor = 3.3f / (1 << 12);
        uint16_t result = adc_read();
        printf("Raw value: 0x%03x, voltage: %f V\n", result, result * conversion_factor);
        sleep_ms(500);
    }

    for (int pin = 0; pin < 30; ++pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_down(pin);
    }

    auto setupOutput = [](uint pin, bool value) {
        gpio_put(pin, value);
        gpio_set_dir(pin, GPIO_OUT);
        gpio_set_slew_rate(pin, GPIO_SLEW_RATE_FAST);
    };

    setupOutput(O_ROMCS_L, true); // must be high in order to enable our ROM
    setupOutput(O_WAIT_L, true);
    setupOutput(O_OE_L, false);

    auto pio = pio0;
    auto sm = pio_claim_unused_sm(pio, true);

    auto offset = pio_add_program(pio, &zx_program);
    pio_sm_config c = zx_program_get_default_config(offset);

    // O_WAIT_L is used by the PIO to set the ZX spectum in wait state
    pio_gpio_init(pio, O_WAIT_L);
    sm_config_set_sideset_pins(&c, O_WAIT_L);
    pio_sm_set_consecutive_pindirs(pio, sm, O_WAIT_L, 1, true);

    // I_MREQ_L is used by the PIO to enable DATA bus
    sm_config_set_jmp_pin(&c, I_MREQ_L);

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

    constexpr uint32_t MREQ_L_BIT = 1 << I_MREQ_L;
    constexpr uint32_t RD_L_BIT = 1 << I_RD_L;
    // constexpr uint32_t OE_L = 1u << O_OE_L;
    constexpr uint32_t M1_L_BIT = 1 << I_M1_L;

    uint8_t rom[0x4000];
    memcpy(rom, zx_rom, sizeof(zx_rom));
    for (int i = sizeof(zx_rom); i < 0x4000; ++i) {
        rom[i] = i;
    }

    puts("Hello there");
    // auto printPins = [](unsigned pins) {
    //     printf("%032b addr: %04x m1 = %b, mreq = %b, rd = %b\n", pins, pins & 0x3fff, !(pins & M1_L_BIT), !(pins & MREQ_L_BIT), !(pins & RD_L_BIT));
    // };
    int lines = 50;
    while (true) {
        // sleep_ms(1000);
        auto bus = pio_sm_get_blocking(pio, sm);
        const uint16_t addr = bus & 0x3fff;
        // unsigned pins = sio_hw->gpio_in;
        // printPins(pins);
        // printPins(bus);
        const bool mreq = !(bus & MREQ_L_BIT);
        const bool rd = !(bus & RD_L_BIT);
        const bool m1 = !(bus & M1_L_BIT);
        (void) m1;
        // const bool mreq1 = !(pins & MREQ_L_BIT);
        // const bool rd1 = !(pins & RD_L_BIT);
        uint32_t output = 0x00000000;
        if (mreq && rd) {
            // sio_hw->gpio_clr = OE_L; // enable DATA bus
            output = 0x00ff00 | rom[addr];
            // if (addr > sizeof(zx_rom)) {
            //     printf("addr: %04x\n", addr);
            // }
            // printf("m1 = %b, mreq = %b,  mreq1 = %b, rd = %b, rd1 = %b, addr: %04x, data: %02x \n", m1, mreq, mreq1, rd, rd1, addr, unsigned(output) & 0xff);
            printf("m1 = %b, mreq = %b,  rd = %b, addr: %04x, data: %02x \n", m1, mreq, rd, addr, unsigned(output) & 0xff);
            if (!--lines) {
                lines = 50;
                getchar();
                puts("\033[2J\033[H");
            }
        }
        pio_sm_put_blocking(pio, sm, output);
    }
    return 0;
}
