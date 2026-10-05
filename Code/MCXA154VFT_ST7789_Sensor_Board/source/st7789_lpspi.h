/*

 */
#ifndef ST7789_LPSPI_H_
#define ST7789_LPSPI_H_

#include <stdint.h>
#include <stdbool.h>
#include "fsl_lpspi.h"

/* ---- Panel geometry -----------------------------------------------------
 * Defaults below assume the same 240x280 IPS panel used elsewhere
 * (esp32-cyd-aquarium project, driven rotated/landscape). Adjust WIDTH/
 * HEIGHT/ROTATION/X_SHIFT/Y_SHIFT if this is a different panel -- these
 * shift values are the usual fix for the 280-wide variant's RAM offset.
 */
#define ST7789_WIDTH      280
#define ST7789_HEIGHT     240
#define ST7789_ROTATION   1        /* 0..3, see ST7789_SetRotation() */
#define ST7789_X_SHIFT    20
#define ST7789_Y_SHIFT    0

/* Common RGB565 colors */
#define ST7789_BLACK      0x0000
#define ST7789_WHITE      0xFFFF
#define ST7789_RED        0xF800
#define ST7789_GREEN      0x07E0
#define ST7789_BLUE       0x001F
#define ST7789_YELLOW     0xFFE0
#define ST7789_CYAN       0x07FF
#define ST7789_MAGENTA    0xF81F

/* LPSPI instance used for the display */
#define ST7789_LPSPI_BASE LPSPI0

/* Call once at startup, after BOARD_InitBootClocks(). Configures LPSPI0,
 * the CS/DC/RST GPIOs, resets and initializes the panel, and clears the
 * screen to black. */
void ST7789_Init(void);

void ST7789_SetRotation(uint8_t rotation);
void ST7789_FillScreen(uint16_t color);
void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);  //New
void ST7789_DrawFastHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color); //New
void ST7789_DrawFastVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color); //New
void ST7789_DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color); //New
void ST7789_FillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color); //New

/* Text uses a built-in 5x7 bitmap font, scaled by 'size' (1 = 5x7 px/char,
 * 2 = 10x14 px/char, etc; max size is 8, see MAX_TEXT_SIZE in the .c file).
 * Wraps to the next line at the panel edge. */
void ST7789_WriteChar(uint16_t x, uint16_t y, char c, uint16_t fgColor, uint16_t bgColor, uint8_t size);
void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, uint16_t fgColor, uint16_t bgColor, uint8_t size);

#endif /* ST7789_LPSPI_H_ */
