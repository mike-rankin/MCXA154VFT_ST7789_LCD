/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MCXA154VFT_I2C_Scanner.c
 * @brief   LPI2C0 bus scanner for the NXP MCXA154VFT (48-pin HVQFN).
 *
 * Hardware:
 *   SDA -> P0_16 (LPI2C0_SDA, pin 43, ALT2)
 *   SCL -> P0_17 (LPI2C0_SCL, pin 44, ALT2)
 *   External 4.7k pull-ups to VDD on both SDA and SCL.
 *
 * Debug console: LPUART0 (P2_0 RX / P0_3 TX) at 115200 baud.
 * Heartbeat LED: P3_14 (pin 28).
 *
 * pin_mux.c (Config Tools) already muxes P0_16/P0_17 to LPI2C0 and enables the
 * input buffer. It does NOT enable an internal pull-up, so the external 4.7k
 * resistors are the only pull-ups -- which is what you want.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_lpi2c.h"
#include "fsl_clock.h"
#include "fsl_port.h"

#define SCAN_LPI2C_BASE   LPI2C0
#define SCAN_BAUDRATE_HZ  100000U

/* Release a stuck bus: if a slave is holding SDA low mid-byte, clock SCL
 * (bit-banged) until it lets go, then issue a STOP. Must be called BEFORE the
 * pins are handed to LPI2C0. Returns true if SDA is high afterwards. */
static bool I2C_BusRecover(void)
{
    /* Temporarily make P0_16/P0_17 plain GPIO (ALT0) */
    PORT_SetPinMux(PORT0, 16U, kPORT_MuxAlt0);
    PORT_SetPinMux(PORT0, 17U, kPORT_MuxAlt0);

    gpio_pin_config_t inCfg  = {kGPIO_DigitalInput, 0U};
    gpio_pin_config_t outCfg = {kGPIO_DigitalOutput, 1U};

    CLOCK_EnableClock(kCLOCK_GateGPIO0);
    RESET_ReleasePeripheralReset(kGPIO0_RST_SHIFT_RSTn);

    GPIO_PinInit(GPIO0, 16U, &inCfg);   /* SDA as input (released) */
    GPIO_PinInit(GPIO0, 17U, &outCfg);  /* SCL as output, driven high */

    for (int i = 0; i < 9 && GPIO_PinRead(GPIO0, 16U) == 0U; i++)
    {
        GPIO_PinWrite(GPIO0, 17U, 0U);
        SDK_DelayAtLeastUs(5, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
        GPIO_PinWrite(GPIO0, 17U, 1U);
        SDK_DelayAtLeastUs(5, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    }
    bool sdaHigh = (GPIO_PinRead(GPIO0, 16U) != 0U);

    /* Hand the pins back to LPI2C0 (ALT2), input buffer on, no internal pulls */
    PORT_SetPinMux(PORT0, 16U, kPORT_MuxAlt2);
    PORT_SetPinMux(PORT0, 17U, kPORT_MuxAlt2);
    PORT0->PCR[16] |= PORT_PCR_IBE(1U);
    PORT0->PCR[17] |= PORT_PCR_IBE(1U);

    return sdaHigh;
}

int main(void)
{
    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    PRINTF("\r\nLPI2C0 bus scanner - MCXA154VFT\r\n");
    PRINTF("SDA = P0_16 (pin 43), SCL = P0_17 (pin 44)\r\n");

    /* Check the idle bus levels (with pull-ups both should read high). */
    if (!I2C_BusRecover())
    {
        PRINTF("WARNING: SDA is stuck low. Check for a shorted line, a missing\r\n"
               "pull-up, or an unpowered/latched-up slave.\r\n");
    }

    /* pin_mux.c releases LPI2C0 from reset but does not attach a clock source
     * or enable the gate -- do both before touching the peripheral. */
    CLOCK_SetClockDiv(kCLOCK_DivLPI2C0, 1U);
    CLOCK_AttachClk(kFRO12M_to_LPI2C0);
    CLOCK_EnableClock(kCLOCK_GateLPI2C0);

    uint32_t srcClk = CLOCK_GetLpi2cClkFreq(0U);
    PRINTF("LPI2C0 functional clock: %u Hz\r\n", (unsigned int)srcClk);

    lpi2c_master_config_t masterConfig;
    LPI2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Hz = SCAN_BAUDRATE_HZ;
    LPI2C_MasterInit(SCAN_LPI2C_BASE, &masterConfig, srcClk);

    PRINTF("Scanning addresses 0x08-0x77 @ %u kHz...\r\n", SCAN_BAUDRATE_HZ / 1000U);

    uint8_t foundCount = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        lpi2c_master_transfer_t xfer = {0};
        xfer.slaveAddress   = addr;
        xfer.direction      = kLPI2C_Write;
        xfer.subaddress     = 0;
        xfer.subaddressSize = 0;
        xfer.data           = NULL;
        xfer.dataSize       = 0;
        xfer.flags          = kLPI2C_TransferDefaultFlag;

        status_t result = LPI2C_MasterTransferBlocking(SCAN_LPI2C_BASE, &xfer);

        if (result == kStatus_Success)
        {
            PRINTF("  0x%02X: ACK - device found!\r\n", addr);
            foundCount++;
        }
    }
    PRINTF("Scan complete. %u device(s) found.\r\n", foundCount);

    while (1)
    {
        /* Slow blink = scan finished (not hung). */
        //GPIO_PinWrite(BOARD_INITPINS_Led_GPIO, BOARD_INITPINS_Led_PIN, 1U);
        SDK_DelayAtLeastUs(500000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
        //GPIO_PinWrite(BOARD_INITPINS_Led_GPIO, BOARD_INITPINS_Led_PIN, 0U);
        SDK_DelayAtLeastUs(500000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
    }
    return 0;
}
