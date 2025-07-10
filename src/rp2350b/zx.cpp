#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/timer.h>
#include <hardware/vreg.h>
#include <pico/multicore.h>

#include "zx.h"

uint8_t *RomPtr = nullptr;
uint16_t RomSize = 0;

namespace {

constexpr uint32_t O_NMI    = 3;
constexpr uint32_t O_HC_CPM = 13;
constexpr uint32_t O_ROMCS = 25;


#define pio pio2
int mreqSM = 0;
uint mreqRxEmptyMask = (1u << (PIO_FSTAT_RXEMPTY_LSB));
uint mreqTxFullMask = (1u << (PIO_FSTAT_TXFULL_LSB));

int iorqSM = 1;
uint iorqRxEmptyMask = (1u << (PIO_FSTAT_RXEMPTY_LSB));
uint iorqTxFullMask = (1u << (PIO_FSTAT_TXFULL_LSB));

void setup_common_pio()
{
    pio_set_gpio_base(pio, PIO_BASE);
    for (int i = PIO_BASE; i < PIO_BASE + 32 ; ++i) {
        if (i == PIO_BASE + DONT_USE)
            continue;

        pio_gpio_init(pio, i);
        gpio_set_dir(i, false);
        gpio_pull_down(i);
    }
    gpio_pull_up(PIO_BASE + O_WAIT_L);
}

void setup_common_config(pio_sm_config *c, int offset, int sm)
{

    pio_sm_set_consecutive_pindirs(pio, sm, PIO_BASE + B_DATA_BASE, 8, false);
    pio_sm_set_consecutive_pindirs(pio, sm, PIO_BASE + O_WAIT_L, 1, true);

    sm_config_set_clkdiv(c, 1);

    sm_config_set_sideset_pins(c, PIO_BASE + O_WAIT_L);
    sm_config_set_in_pin_base(c, PIO_BASE);
    sm_config_set_in_shift(c, true, true, 32);

    sm_config_set_out_pin_base(c, PIO_BASE);
    sm_config_set_out_pin_count(c, 8);
    sm_config_set_out_shift(c, true, true, 24);

    // if /WR is high we have a /RD
    sm_config_set_jmp_pin(c, PIO_BASE + I_WR_L);

    pio_sm_init(pio, sm, offset, c);
    pio_sm_set_enabled(pio, sm, true);
    pio_sm_drain_tx_fifo(pio, sm);
}

void setup_zx_mreq_pio()
{
    mreqSM = pio_claim_unused_sm(pio, true);
    mreqRxEmptyMask <<= mreqSM;
    mreqTxFullMask <<= mreqSM;

    auto offset = pio_add_program(pio, &zx_mreq_program);
    pio_sm_config c = zx_mreq_program_get_default_config(offset);
    setup_common_config(&c, offset, mreqSM);
}

void setup_zx_iorq_pio()
{
    iorqSM = pio_claim_unused_sm(pio, true);
    iorqRxEmptyMask <<= iorqSM;
    iorqTxFullMask <<= iorqSM;
    auto offset = pio_add_program(pio, &zx_iorq_program);
    pio_sm_config c = zx_iorq_program_get_default_config(offset);
    setup_common_config(&c, offset, iorqSM);
}


void setOutGpio(int pos)
{
    gpio_init(pos);
    gpio_set_dir(pos, GPIO_OUT);
    gpio_put(pos, false);
}


constexpr uint32_t ZxRdMask = 1u << I_RD_L;
constexpr uint32_t ZxWrMask = 1u << I_WR_L;
constexpr uint32_t MreqMask = 1u << I_MREQ_L;
constexpr uint32_t IOrqMask = 1u << I_IORQ_L;

bool Romcs = true;
constexpr uint32_t RomRead = (0xc000 << I_ADDR_BASE) | MreqMask | ZxRdMask;

bool inline  __time_critical_func(zx_mreq)()
{
    if (pio->fstat & mreqRxEmptyMask)
        return false;
    const uint32_t bus = pio->rxf[mreqSM];

    const uint16_t addr = bus >> 16;
    uint32_t outData = 0;
    if (Romcs && !(bus & RomRead)) {
        outData = RomPtr[addr];
    } else {
        if (!(bus & MreqMask)) {
            switch(addr) {
            case 0x0066: // NMI
            case 0x0008: // shadow rom error handling
            case 0x1708: // shadow rom error handling
                Romcs = true;
                gpio_put(O_ROMCS, Romcs);
                outData = RomPtr[addr];
                break;
            }
        } else {
            // IORQ
            switch (addr & 0xff)
            {
            case 0x1f: // Kempston joystick
            case 0x7F: // Fuller joystick
                outData = 0; // TODO: handle Kempston joystick
                break;
            case 0xFE:
                outData = 0; // TODO: Sinclair/Cursor joysticks & TAPE
                break;
            default:
                break;
            }
        }
    }

    pio->txf[mreqSM] = outData;
    return true;
}

void inline  __time_critical_func(zx_iorq)()
{
    if (pio->fstat & iorqRxEmptyMask)
        return;
    const uint32_t bus = pio->rxf[iorqSM];

    const uint16_t addr = bus >> 16;
    uint32_t outData = 0;
    if (!(bus&ZxRdMask)) {
        switch (addr & 0xff)
        {
        case 0x1f: // Kempston joystick
        case 0x7F: // Fuller joystick
            outData = 0; // TODO: handle Kempston joystick
            break;
        case 0xFE:
            outData = 0; // TODO: Sinclair/Cursor joysticks & TAPE
            break;
        default:
            break;
        }
    } else if (addr == 3)  {
        // TODO Handle
    }

    pio->txf[iorqSM] = outData;
}

} // namespace {

void __time_critical_func(zx_main)()
{
    // vreg_disable_voltage_limit();
    // vreg_set_voltage(VREG_VOLTAGE_1_35);
    // set_sys_clock_khz(300000, true);

    // disable all interrupts
    irq_set_mask_enabled(0xFFFFFFFF, false);

    setup_common_pio();

    setup_zx_mreq_pio();
    setup_zx_iorq_pio();

    setOutGpio(O_ROMCS);
    setOutGpio(O_HC_CPM);
    gpio_put(O_ROMCS, Romcs);

    // wait for stdio
    multicore_fifo_pop_blocking();

    for (uint16_t i = 0; i < RomSize; ++RomSize)
        (void)RomPtr[i];

    while(true) {
        if (!zx_mreq()) {
             zx_iorq();
        }
    }
}
