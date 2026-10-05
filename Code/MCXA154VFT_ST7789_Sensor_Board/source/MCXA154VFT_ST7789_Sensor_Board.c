/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    MCXA154VFT_ST7789_Sensor_Board.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "st7789_lpspi.h"
#include "sht40_lpi2c.h"
#include "lsm6ds3_lpi2c.h"
#include "fsl_gpio.h"

#define COUNT_X     20
#define COUNT_Y     100
#define COUNT_SIZE  4                       /* 4x scale -> 20x28 px per digit */
#define COUNT_W     (20 * COUNT_SIZE)       /* room for 2 digits ("10") */
#define COUNT_H     (7 * COUNT_SIZE)

#define COL_BG      0x0000   /* dark background, was 0x10A8 */
#define COL_CARD    0x4208    /* slightly lighter card, was 0x214E */
#define COL_LABEL   0xDEFB    /* dim grey for captions */
#define COL_VALUE   0xFFFF   /* white values */
#define COL_ACCENT  0x07FF   /* cyan accent */

extern uint32_t _etext, _data, _edata, _ebss;
#define FLASH_SIZE_BYTES  (256U * 1024U)   /* MCXA154 */
#define RAM_SIZE_BYTES    (64U * 1024U)

static volatile uint32_t g_ms = 0;
static void FormatFixed1(char *buf, size_t bufSize, float value)
{
    bool neg = value < 0.0f;
    if (neg) { value = -value; }
    int whole = (int)value;
    int frac  = (int)((value - (float)whole) * 10.0f + 0.5f);
    if (frac >= 10) { frac -= 10; whole += 1; }
    snprintf(buf, bufSize, "%s%d.%d", neg ? "-" : "", whole, frac);
}

#define LED_ON      0U      /* LED is active-low */
#define LED_OFF     1U
#define LED_ON_MS   500U

static volatile uint32_t g_ledMsLeft = 0;

extern uint32_t _vStackTop;            /* top of RAM / stack start, check the name in your .ld */
#define STACK_PAINT  0xA5A5A5A5U

static uint32_t *g_paintStart;
static uint32_t *g_paintEnd;

static void StackPaint(void)
{
    uint32_t *p  = &_ebss;
    uint32_t *sp = (uint32_t *)(__get_MSP() - 256U);   /* margin below the current SP */
    g_paintStart = p;
    g_paintEnd   = sp;
    while (p < sp) { *p++ = STACK_PAINT; }
}

/* static RAM + deepest stack reached so far, in bytes */
static uint32_t RamUsedBytes(void)
{
    uint32_t *p = g_paintStart;
    while (p < g_paintEnd && *p == STACK_PAINT) { p++; }   /* first overwritten word */

    uint32_t staticUsed = (uint32_t)&_ebss - (uint32_t)&_data;
    uint32_t stackUsed  = (uint32_t)&_vStackTop - (uint32_t)p;
    return staticUsed + stackUsed;
}


static uint32_t FlashUsedPct(void)
{
    /* code + constants, plus the initial values of .data stored in flash */
    uint32_t used = (uint32_t)&_etext + ((uint32_t)&_edata - (uint32_t)&_data);
    return (100U * used) / FLASH_SIZE_BYTES;
}





//Led on timer
void SysTick_Handler(void)
{
	g_ms++;
    if (g_ledMsLeft > 0U)
    {
        if (--g_ledMsLeft == 0U)
        {
            GPIO_PinWrite(BOARD_INITPINS_Led_GPIO, BOARD_INITPINS_Led_GPIO_PIN, LED_OFF);
        }
    }
}

//Interrupt handler
void GPIO2_IRQHandler(void)
{
    uint32_t flags = GPIO_GpioGetInterruptFlags(BOARD_INITPINS_PushButton_GPIO);

    if (flags & BOARD_INITPINS_PushButton_GPIO_PIN_MASK)
    {
        GPIO_GpioClearInterruptFlags(BOARD_INITPINS_PushButton_GPIO,
                                     BOARD_INITPINS_PushButton_GPIO_PIN_MASK);
        GPIO_PinWrite(BOARD_INITPINS_Led_GPIO, BOARD_INITPINS_Led_GPIO_PIN, LED_ON);
        g_ledMsLeft = LED_ON_MS;
    }
    SDK_ISR_EXIT_BARRIER;
}

int main(void)
{
	StackPaint();
	uint32_t flashPct = FlashUsedPct();

    BOARD_InitBootPins();
    GPIO_PinWrite(BOARD_INITPINS_Led_GPIO, BOARD_INITPINS_Led_GPIO_PIN, LED_OFF);
    GPIO_GpioClearInterruptFlags(BOARD_INITPINS_PushButton_GPIO, BOARD_INITPINS_PushButton_GPIO_PIN_MASK);
    EnableIRQ(GPIO2_IRQn);
    //BOARD_InitBootClocks(); //External crystal not working
    BOARD_BootClockFRO48M();  //For internal clock
    SysTick_Config(SystemCoreClock / 1000U);
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    BOARD_InitDebugConsole();
#endif

    ST7789_Init();
    SHT40_Init();
    LSM6DS3_Init();

    ST7789_FillRect(0, 0, 280, 240, COL_BG);
    ST7789_WriteString(50, 15, "NXP MCXA154VFT", COL_ACCENT, COL_BG, 2);
    ST7789_FillRoundRect(10, 45, 260, 65, 10, COL_CARD);
    ST7789_FillRoundRect(10, 125, 260, 90, 10, COL_CARD);

    ST7789_WriteString(20, 50, "ENVIRONMENT", COL_LABEL, COL_CARD, 2);
    ST7789_WriteString(20, 130, "MOTION", COL_LABEL, COL_CARD, 2);

    ST7789_WriteString(220, 225, "Ver1.0", ST7789_GREEN, COL_BG, 1);

    char tempStr[16], humStr[16], gxStr[12], gyStr[12], gzStr[12], line[40];
    float tempC, humidity, gx, gy, gz;

    float bx = 0, by = 0, bz = 0;
    for (int i = 0; i < 100; i++)
    {
        if (LSM6DS3_ReadGyro(&gx, &gy, &gz)) { bx += gx; by += gy; bz += gz; }
        SDK_DelayAtLeastUs(10000, SystemCoreClock);
    }
    bx /= 100.0f; by /= 100.0f; bz /= 100.0f;

    float angX = 0, angY = 0, angZ = 0;
    uint32_t lastMs = g_ms;

    uint32_t lastDisp = 0;

    while(1)
       {
        /* fast: integrate the gyro every pass */
        if (LSM6DS3_ReadGyro(&gx, &gy, &gz))
        {
         uint32_t now = g_ms;
         float dt = (float)(now - lastMs) * 0.001f;
         lastMs = now;
         angX += (gx - bx) * dt;
         angY += (gy - by) * dt;
         angZ += (gz - bz) * dt;
        }

        /* slow: update the display every 200 ms */
        if ((g_ms - lastDisp) >= 200U)
        {
         lastDisp = g_ms;

         if (SHT40_ReadMeasurement(&tempC, &humidity))
         {
          FormatFixed1(tempStr, sizeof(tempStr), tempC);
          FormatFixed1(humStr, sizeof(humStr), humidity);
          snprintf(line, sizeof(line), "Temperature: %s ^C   ", tempStr);
          ST7789_WriteString(20, 70, line, COL_VALUE, COL_CARD, 2);
          snprintf(line, sizeof(line), "Humidity:    %s %%  ", humStr);
          ST7789_WriteString(20, 90, line, COL_VALUE, COL_CARD, 2);
         }

         FormatFixed1(gxStr, sizeof(gxStr), angX);
         FormatFixed1(gyStr, sizeof(gyStr), angY);
         FormatFixed1(gzStr, sizeof(gzStr), angZ);

         snprintf(line, sizeof(line), "x: %s ^      ", gxStr);
         ST7789_WriteString(20, 150, line, COL_VALUE, COL_CARD, 2);
         snprintf(line, sizeof(line), "y: %s ^      ", gyStr);
         ST7789_WriteString(20, 170, line, COL_VALUE, COL_CARD, 2);
         snprintf(line, sizeof(line), "z: %s ^      ", gzStr);
         ST7789_WriteString(20, 190, line, COL_VALUE, COL_CARD, 2);

         //snprintf(line, sizeof(line), "FLASH %u%%  RAM %u%%  ",(unsigned)flashPct, (unsigned)RamUsedPct());
         //ST7789_WriteString(60, 225, line, ST7789_GREEN, COL_BG, 1);

         //snprintf(line, sizeof(line), "D%08lX E%08lX B%08lX",
                  //(unsigned long)&_data, (unsigned long)&_edata, (unsigned long)&_ebss);
         //ST7789_WriteString(60, 225, line, ST7789_GREEN, COL_BG, 1);

         uint32_t ramBytes = RamUsedBytes();
                  snprintf(line, sizeof(line), "FLASH %u%%  RAM %u%%  ",
                           (unsigned)flashPct,
                           (unsigned)((100U * ramBytes) / RAM_SIZE_BYTES),
                           (unsigned)ramBytes);
         ST7789_WriteString(30, 225, line, ST7789_GREEN, COL_BG, 1);

        }

        SDK_DelayAtLeastUs(5000, SystemCoreClock);
       }
    return 0 ;
}
