/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MCXA154VFT_External_Crystal_Test.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

//#define BOARD_INITPINS_Led_GPIO GPIO1
//#define BOARD_INITPINS_Led_PIN  5



int main(void) {

    BOARD_InitBootPins();

    BOARD_InitBootClocks();     //For external crystal
    //BOARD_BootClockFRO48M();  //For internal clock

    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    PRINTF("Hello World\r\n");
    //volatile static int i = 0 ;
    while(1) {

   	 //Turn LED on
     GPIO_PinWrite(BOARD_INITPINS_Led_GPIO, BOARD_INITPINS_Led_PIN, 1U);
     //GPIO_PinWrite(GPIO1, 5, 1U);

     //SDK_DelayAtLeastUs(1000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
     SDK_DelayAtLeastUs(500000U, SystemCoreClock);   // 500 ms  For External clock

     //Turn LED off
     GPIO_PinWrite(BOARD_INITPINS_Led_GPIO, BOARD_INITPINS_Led_PIN, 0U);
     //GPIO_PinWrite(GPIO1, 5, 0U);

     //SDK_DelayAtLeastUs(1000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY);
     SDK_DelayAtLeastUs(500000U, SystemCoreClock);   // 500 ms

     __asm volatile ("nop");

    }
    return 0 ;
}
