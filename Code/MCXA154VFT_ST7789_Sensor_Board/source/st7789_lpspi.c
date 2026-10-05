/*

 */
#include "st7789_lpspi.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "fsl_common.h"
#include "clock_config.h"
#include <stdlib.h>   /* abs() */   //New

/* ---- Pin assignments (must match board/pin_mux.c) ------------------ */
#define LCD_CS_GPIO  GPIO1
#define LCD_CS_PIN   3U
#define LCD_DC_GPIO  GPIO3
#define LCD_DC_PIN   10U
#define LCD_RST_GPIO GPIO3
#define LCD_RST_PIN  9U

#define LCD_CS_LOW()    GPIO_PinWrite(LCD_CS_GPIO, LCD_CS_PIN, 0U)
#define LCD_CS_HIGH()   GPIO_PinWrite(LCD_CS_GPIO, LCD_CS_PIN, 1U)
#define LCD_DC_LOW()    GPIO_PinWrite(LCD_DC_GPIO, LCD_DC_PIN, 0U)   /* command */
#define LCD_DC_HIGH()   GPIO_PinWrite(LCD_DC_GPIO, LCD_DC_PIN, 1U)   /* data    */
#define LCD_RST_LOW()   GPIO_PinWrite(LCD_RST_GPIO, LCD_RST_PIN, 0U)
#define LCD_RST_HIGH()  GPIO_PinWrite(LCD_RST_GPIO, LCD_RST_PIN, 1U)

/* ---- ST7789 command set (subset used here) -------------------------- */
#define ST7789_SLPOUT  0x11
#define ST7789_INVON   0x21
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_MADCTL  0x36
#define ST7789_COLMOD  0x3A
#define ST7789_NORON   0x13

#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_RGB 0x00

#define ST7789_COLOR_MODE_16BIT 0x55

/* ---- Built-in 5x7 font table ----------------------------------------- */
#include "font5x7.inc"

static void delay_ms(uint32_t ms)
{
    /* Coarse busy-wait delay; good enough for init/reset timing. */
    SDK_DelayAtLeastUs(ms * 1000U, SystemCoreClock);
}

/* ---- Low level SPI transfer ------------------------------------------ */
static void ST7789_SpiWrite(const uint8_t *data, size_t len)
{
    lpspi_transfer_t transfer = {0};
    transfer.txData      = data;
    transfer.rxData       = NULL;
    transfer.dataSize     = len;
    transfer.configFlags  = kLPSPI_MasterPcs0 | kLPSPI_MasterPcsContinuous;

    LPSPI_MasterTransferBlocking(ST7789_LPSPI_BASE, &transfer);
}

static void ST7789_WriteCommand(uint8_t cmd)
{
    LCD_CS_LOW();
    LCD_DC_LOW();
    ST7789_SpiWrite(&cmd, 1);
    LCD_CS_HIGH();
}

static void ST7789_WriteDataBuf(const uint8_t *buf, size_t len)
{
    LCD_CS_LOW();
    LCD_DC_HIGH();
    ST7789_SpiWrite(buf, len);
    LCD_CS_HIGH();
}

static void ST7789_WriteData8(uint8_t data)
{
    ST7789_WriteDataBuf(&data, 1);
}

static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint16_t xs = x0 + ST7789_X_SHIFT, xe = x1 + ST7789_X_SHIFT;
    uint16_t ys = y0 + ST7789_Y_SHIFT, ye = y1 + ST7789_Y_SHIFT;

    ST7789_WriteCommand(ST7789_CASET);
    uint8_t colData[4] = {(uint8_t)(xs >> 8), (uint8_t)(xs & 0xFF), (uint8_t)(xe >> 8), (uint8_t)(xe & 0xFF)};
    ST7789_WriteDataBuf(colData, sizeof(colData));

    ST7789_WriteCommand(ST7789_RASET);
    uint8_t rowData[4] = {(uint8_t)(ys >> 8), (uint8_t)(ys & 0xFF), (uint8_t)(ye >> 8), (uint8_t)(ye & 0xFF)};
    ST7789_WriteDataBuf(rowData, sizeof(rowData));

    ST7789_WriteCommand(ST7789_RAMWR);
}

void ST7789_SetRotation(uint8_t rotation)
{
    uint8_t madctl = 0;
    switch (rotation)
    {
        case 0: madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB; break;
        case 1: madctl = ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB; break;
        case 2: madctl = ST7789_MADCTL_RGB; break;
        case 3: madctl = ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB; break;
        default: return;
    }
    ST7789_WriteCommand(ST7789_MADCTL);
    ST7789_WriteData8(madctl);
}

void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT)
    {
        return;
    }
    if ((uint32_t)(x + w) > ST7789_WIDTH)
    {
        w = ST7789_WIDTH - x;
    }
    if ((uint32_t)(y + h) > ST7789_HEIGHT)
    {
        h = ST7789_HEIGHT - y;
    }

    ST7789_SetAddressWindow(x, y, x + w - 1, y + h - 1);

    /* Send the fill color a scanline at a time, direct from a small stack
     * buffer -- keeps RAM use low and still gives decent throughput. */
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    uint8_t line[2 * 64];
    uint32_t lineLen = (w < 64) ? w : 64;
    for (uint32_t i = 0; i < lineLen; i++)
    {
        line[2 * i] = hi;
        line[2 * i + 1] = lo;
    }

    LCD_CS_LOW();
    LCD_DC_HIGH();
    uint32_t pixelsRemaining = (uint32_t)w * (uint32_t)h;
    while (pixelsRemaining > 0)
    {
        uint32_t chunk = (pixelsRemaining < lineLen) ? pixelsRemaining : lineLen;
        ST7789_SpiWrite(line, (size_t)chunk * 2U);
        pixelsRemaining -= chunk;
    }
    LCD_CS_HIGH();
}

void ST7789_FillScreen(uint16_t color)
{
    ST7789_FillRect(0, 0, ST7789_WIDTH, ST7789_HEIGHT, color);
}

void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT)
    {
        return;
    }
    ST7789_SetAddressWindow(x, y, x, y);
    uint8_t data[2] = {(uint8_t)(color >> 8), (uint8_t)(color & 0xFF)};
    LCD_CS_LOW();
    LCD_DC_HIGH();
    ST7789_SpiWrite(data, sizeof(data));
    LCD_CS_HIGH();
}


void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint16_t swap;
    bool steep = abs((int32_t)y1 - (int32_t)y0) > abs((int32_t)x1 - (int32_t)x0);

    if (steep)
    {
        swap = x0; x0 = y0; y0 = swap;
        swap = x1; x1 = y1; y1 = swap;
    }

    if (x0 > x1)
    {
        swap = x0; x0 = x1; x1 = swap;
        swap = y0; y0 = y1; y1 = swap;
    }

    int32_t dx = (int32_t)x1 - (int32_t)x0;
    int32_t dy = abs((int32_t)y1 - (int32_t)y0);
    int32_t err = dx / 2;
    int32_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++)
    {
        if (steep)
        {
            ST7789_DrawPixel(y0, x0, color);
        }
        else
        {
            ST7789_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0)
        {
            y0 = (uint16_t)((int32_t)y0 + ystep);
            err += dx;
        }
    }
}

void ST7789_DrawFastHLine(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
    if (w == 0) { return; }
    ST7789_FillRect(x, y, w, 1, color);   /* FillRect already clips to the panel */
}

void ST7789_DrawFastVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
    if (h == 0) { return; }
    ST7789_FillRect(x, y, 1, h, color);
}

/* Draws the selected quarter-circle arcs of a rounded corner (midpoint
 * circle algorithm). cornername bits: 0x1 = top-left, 0x2 = top-right,
 * 0x4 = bottom-right, 0x8 = bottom-left. */
static void ST7789_DrawCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, uint16_t color)
{
    int32_t f     = 1 - r;
    int32_t ddF_x = 1;
    int32_t ddF_y = -2 * r;
    int32_t x     = 0;

    while (x < r)
    {
        if (f >= 0)
        {
            r--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x4)
        {
            ST7789_DrawPixel((uint16_t)(x0 + x), (uint16_t)(y0 + r), color);
            ST7789_DrawPixel((uint16_t)(x0 + r), (uint16_t)(y0 + x), color);
        }
        if (cornername & 0x2)
        {
            ST7789_DrawPixel((uint16_t)(x0 + x), (uint16_t)(y0 - r), color);
            ST7789_DrawPixel((uint16_t)(x0 + r), (uint16_t)(y0 - x), color);
        }
        if (cornername & 0x8)
        {
            ST7789_DrawPixel((uint16_t)(x0 - r), (uint16_t)(y0 + x), color);
            ST7789_DrawPixel((uint16_t)(x0 - x), (uint16_t)(y0 + r), color);
        }
        if (cornername & 0x1)
        {
            ST7789_DrawPixel((uint16_t)(x0 - r), (uint16_t)(y0 - x), color);
            ST7789_DrawPixel((uint16_t)(x0 - x), (uint16_t)(y0 - r), color);
        }
    }
}

void ST7789_DrawRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
    if (w == 0 || h == 0) { return; }

    /* Radius can't exceed half of the shorter side */
    uint16_t maxR = ((w < h) ? w : h) / 2U;
    if (r > maxR) { r = maxR; }

    ST7789_DrawFastHLine(x + r,     y,         w - 2U * r, color);  /* Top    */
    ST7789_DrawFastHLine(x + r,     y + h - 1, w - 2U * r, color);  /* Bottom */
    ST7789_DrawFastVLine(x,         y + r,     h - 2U * r, color);  /* Left   */
    ST7789_DrawFastVLine(x + w - 1, y + r,     h - 2U * r, color);  /* Right  */

    ST7789_DrawCircleHelper(x + r,         y + r,         r, 0x1, color);  /* TL */
    ST7789_DrawCircleHelper(x + w - r - 1, y + r,         r, 0x2, color);  /* TR */
    ST7789_DrawCircleHelper(x + w - r - 1, y + h - r - 1, r, 0x4, color);  /* BR */
    ST7789_DrawCircleHelper(x + r,         y + h - r - 1, r, 0x8, color);  /* BL */
}


/* Fills the top and/or bottom half of a rounded end using horizontal runs.
 * NOTE: the cornername bits differ from ST7789_DrawCircleHelper above:
 * 0x1 = bottom half, 0x2 = top half. 'delta' is the extra width between
 * the two corners (i.e. the straight section of the rectangle). */
static void ST7789_FillCircleHelper(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint16_t color)
{
    int32_t f     = 1 - r;
    int32_t ddF_x = 1;
    int32_t ddF_y = -r - r;
    int32_t y     = 0;

    delta++;

    while (y < r)
    {
        if (f >= 0)
        {
            if (cornername & 0x1)
            {
                ST7789_DrawFastHLine((uint16_t)(x0 - y), (uint16_t)(y0 + r), (uint16_t)(y + y + delta), color);
            }
            if (cornername & 0x2)
            {
                ST7789_DrawFastHLine((uint16_t)(x0 - y), (uint16_t)(y0 - r), (uint16_t)(y + y + delta), color);
            }
            r--;
            ddF_y += 2;
            f     += ddF_y;
        }

        y++;
        ddF_x += 2;
        f     += ddF_x;

        if (cornername & 0x1)
        {
            ST7789_DrawFastHLine((uint16_t)(x0 - r), (uint16_t)(y0 + y), (uint16_t)(r + r + delta), color);
        }
        if (cornername & 0x2)
        {
            ST7789_DrawFastHLine((uint16_t)(x0 - r), (uint16_t)(y0 - y), (uint16_t)(r + r + delta), color);
        }
    }
}

void ST7789_FillRoundRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t r, uint16_t color)
{
    if (w == 0 || h == 0) { return; }

    /* Radius can't exceed half of the shorter side */
    uint16_t maxR = ((w < h) ? w : h) / 2U;
    if (r > maxR) { r = maxR; }

    /* Middle band, full width */
    ST7789_FillRect(x, y + r, w, h - 2U * r, color);

    /* Bottom and top rounded ends */
    ST7789_FillCircleHelper(x + r, y + h - r - 1, r, 0x1, (int32_t)w - 2 * r - 1, color);
    ST7789_FillCircleHelper(x + r, y + r,         r, 0x2, (int32_t)w - 2 * r - 1, color);
}

/* Max text 'size' this buffer supports (5 * MAX_TEXT_SIZE pixels/row * 2 bytes). */
#define MAX_TEXT_SIZE 8U

void ST7789_WriteChar(uint16_t x, uint16_t y, char c, uint16_t fgColor, uint16_t bgColor, uint8_t size)
{
    if (size == 0 || size > MAX_TEXT_SIZE)
    {
        size = 1;
    }
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT)
    {
        return;
    }
    if ((uint8_t)c < FONT5X7_FIRST_CHAR || (uint8_t)c > FONT5X7_LAST_CHAR)
    {
        c = ' ';
    }
    const uint8_t *rows = g_font5x7[(uint8_t)c - FONT5X7_FIRST_CHAR];

    uint16_t charW = FONT5X7_WIDTH * size;
    uint16_t charH = FONT5X7_HEIGHT * size;
    if ((uint32_t)(x + charW) > ST7789_WIDTH)
    {
        charW = ST7789_WIDTH - x;
    }
    if ((uint32_t)(y + charH) > ST7789_HEIGHT)
    {
        charH = ST7789_HEIGHT - y;
    }
    if (charW == 0 || charH == 0)
    {
        return;
    }

    /* Set the address window once for the whole glyph, then stream each
     * (vertically-repeated) pixel row as a single SPI burst, instead of
     * re-issuing CASET/RASET/RAMWR for every individual pixel -- that
     * per-pixel address-window overhead was the main cause of slow text
     * rendering. */
    ST7789_SetAddressWindow(x, y, x + charW - 1, y + charH - 1);

    uint8_t rowBuf[2 * FONT5X7_WIDTH * MAX_TEXT_SIZE];

    LCD_CS_LOW();
    LCD_DC_HIGH();
    for (uint8_t row = 0; row < FONT5X7_HEIGHT; row++)
    {
        if ((uint16_t)(row * size) >= charH)
        {
            break;
        }
        uint8_t bits = rows[row];
        uint16_t idx = 0;
        for (uint8_t col = 0; col < FONT5X7_WIDTH && idx < (charW * 2U); col++)
        {
            bool set = (bits & (1U << (FONT5X7_WIDTH - 1U - col))) != 0U;
            uint16_t color = set ? fgColor : bgColor;
            uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
            for (uint8_t rep = 0; rep < size && idx < (charW * 2U); rep++)
            {
                rowBuf[idx++] = hi;
                rowBuf[idx++] = lo;
            }
        }
        for (uint8_t line = 0; line < size; line++)
        {
            if ((uint16_t)(row * size + line) >= charH)
            {
                break;
            }
            ST7789_SpiWrite(rowBuf, idx);
        }
    }
    LCD_CS_HIGH();
}

void ST7789_WriteString(uint16_t x, uint16_t y, const char *str, uint16_t fgColor, uint16_t bgColor, uint8_t size)
{
    if (size == 0)
    {
        size = 1;
    }
    uint16_t cursorX = x, cursorY = y;
    uint16_t advance = (FONT5X7_WIDTH + 1U) * size; /* +1 column of spacing */

    while (*str != '\0')
    {
        if (*str == '\n' || (cursorX + advance) > ST7789_WIDTH)
        {
            cursorX = x;
            cursorY += (FONT5X7_HEIGHT + 1U) * size;
            if (*str == '\n')
            {
                str++;
                continue;
            }
        }
        if (cursorY + FONT5X7_HEIGHT * size > ST7789_HEIGHT)
        {
            break;
        }
        ST7789_WriteChar(cursorX, cursorY, *str, fgColor, bgColor, size);
        cursorX += advance;
        str++;
    }
}

/* ---- Peripheral bring-up --------------------------------------------- */
static void ST7789_LpspiInit(void)
{
    /* pin_mux.c releases LPSPI0 out of reset but (like LPI2C0 in this
     * project) does not attach a clock source or enable the peripheral
     * clock gate -- do both here first.
     *
     * Use FRO_HF_DIV (96 MHz in the BOARD_BootClockFRO96M() config this
     * project boots with) rather than FRO12M: FRO12M can only ever give
     * LPSPI0 a source clock of 12 MHz, which caps the achievable SCK well
     * below what the ST7789 can accept and was the main reason drawing
     * felt slow. */
    CLOCK_AttachClk(kFRO_HF_DIV_to_LPSPI0);
    CLOCK_SetClockDiv(kCLOCK_DivLPSPI0, 1U);
    CLOCK_EnableClock(kCLOCK_GateLPSPI0);

    lpspi_master_config_t masterConfig;
    LPSPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate     = 20000000U; /* ST7789 supports well over this; back off if you see glitches */
    masterConfig.bitsPerFrame = 8U;
    masterConfig.cpol         = kLPSPI_ClockPolarityActiveHigh;
    masterConfig.cpha         = kLPSPI_ClockPhaseFirstEdge;
    masterConfig.direction    = kLPSPI_MsbFirst;
    masterConfig.pinCfg       = kLPSPI_SdiInSdoOut;
    masterConfig.whichPcs     = kLPSPI_Pcs0;
    /* PCS0 is configured here for the driver's internal bookkeeping only --
     * P3_11 is wired as a plain GPIO (see LCD_CS_* macros above), not routed
     * to the LPSPI0_PCS0 pin, so the hardware chip-select is unused. */

    LPSPI_MasterInit(ST7789_LPSPI_BASE, &masterConfig, CLOCK_GetLpspiClkFreq(0U));
}

static void ST7789_GpioInit(void)
{
    gpio_pin_config_t outConfig = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic  = 0U,
    };

    /* LCD_CS (P3_11) is already configured as a GPIO output by
     * board/pin_mux.c; re-init here as well is harmless and keeps this
     * driver self-contained if pin_mux.c changes. */
    GPIO_PinInit(LCD_CS_GPIO, LCD_CS_PIN, &outConfig);

    /* LCD_DC (P3_10) and LCD_RST (P3_9) are pin-muxed to GPIO (ALT0) in
     * pin_mux.c but not yet given a GPIO direction -- do that here. */
    GPIO_PinInit(LCD_DC_GPIO, LCD_DC_PIN, &outConfig);
    GPIO_PinInit(LCD_RST_GPIO, LCD_RST_PIN, &outConfig);

    LCD_CS_HIGH();
}

void ST7789_Init(void)
{
    ST7789_GpioInit();
    ST7789_LpspiInit();

    /* Hardware reset */
    LCD_RST_HIGH();
    delay_ms(25);
    LCD_RST_LOW();
    delay_ms(25);
    LCD_RST_HIGH();
    delay_ms(50);

    ST7789_WriteCommand(ST7789_COLMOD);
    ST7789_WriteData8(ST7789_COLOR_MODE_16BIT);

    ST7789_WriteCommand(0xB2); /* Porch control */
    {
        uint8_t d[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
        ST7789_WriteDataBuf(d, sizeof(d));
    }

    ST7789_SetRotation(ST7789_ROTATION);

    ST7789_WriteCommand(0xB7); /* Gate control */
    ST7789_WriteData8(0x35);
    ST7789_WriteCommand(0xBB); /* VCOM setting */
    ST7789_WriteData8(0x19);
    ST7789_WriteCommand(0xC0); /* LCM control */
    ST7789_WriteData8(0x2C);
    ST7789_WriteCommand(0xC2); /* VDV/VRH command enable */
    ST7789_WriteData8(0x01);
    ST7789_WriteCommand(0xC3); /* VRH set */
    ST7789_WriteData8(0x12);
    ST7789_WriteCommand(0xC4); /* VDV set */
    ST7789_WriteData8(0x20);
    ST7789_WriteCommand(0xC6); /* Frame rate control, 60 Hz */
    ST7789_WriteData8(0x0F);
    ST7789_WriteCommand(0xD0); /* Power control */
    {
        uint8_t d[] = {0xA4, 0xA1};
        ST7789_WriteDataBuf(d, sizeof(d));
    }

    ST7789_WriteCommand(0xE0); /* Positive voltage gamma control */
    {
        uint8_t d[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
        ST7789_WriteDataBuf(d, sizeof(d));
    }
    ST7789_WriteCommand(0xE1); /* Negative voltage gamma control */
    {
        uint8_t d[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
        ST7789_WriteDataBuf(d, sizeof(d));
    }

    ST7789_WriteCommand(ST7789_INVON);
    ST7789_WriteCommand(ST7789_SLPOUT);
    ST7789_WriteCommand(ST7789_NORON);
    ST7789_WriteCommand(ST7789_DISPON);

    delay_ms(50);
    ST7789_FillScreen(ST7789_BLACK);
}
