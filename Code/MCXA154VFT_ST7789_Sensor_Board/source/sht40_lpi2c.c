#include "sht40_lpi2c.h"
#include "fsl_lpi2c.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "clock_config.h"

#define SHT40_LPI2C_BASE       LPI2C0
#define SHT40_CMD_MEASURE_HIGH 0xFDU  /* high-repeatability, no heater */

/* Sensirion CRC-8: poly 0x31, init 0xFF -- same checksum used across their
 * I2C sensor line (matches what SEN66 uses). */
static uint8_t SHT40_Crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFFU;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8U; b++)
        {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

void SHT40_Init(void)
{
    /* pin_mux.c releases LPI2C0 out of reset and mux's P0_16/P0_17 to
     * LPI2C0_SDA/SCL, but (like LPSPI0 in this project) doesn't attach a
     * functional clock or enable the peripheral clock gate -- do both
     * here first, or this hangs on the first transfer exactly like the
     * SEN66/FRDM-MCXA153 project did. */
    CLOCK_AttachClk(kFRO12M_to_LPI2C0);   /* if this enum doesn't exist in
                                              fsl_clock.h for this SDK build,
                                              check clock_config.h for the
                                              actual LPI2C0 clock source name */
    CLOCK_SetClockDiv(kCLOCK_DivLPI2C0, 1U);
    CLOCK_EnableClock(kCLOCK_GateLPI2C0);

    lpi2c_master_config_t masterConfig;
    LPI2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Hz = 100000U;

    LPI2C_MasterInit(SHT40_LPI2C_BASE, &masterConfig, CLOCK_GetLpi2cClkFreq(0U));
}

bool SHT40_ReadMeasurement(float *tempC, float *humidityPct)
{
    lpi2c_master_transfer_t xfer = {0};
    uint8_t cmd = SHT40_CMD_MEASURE_HIGH;
    uint8_t raw[6];
    status_t status;

    /* Trigger the measurement */
    xfer.slaveAddress   = SHT40_I2C_ADDR;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = 0;
    xfer.subaddressSize = 0;
    xfer.data           = &cmd;
    xfer.dataSize       = 1;
    xfer.flags          = kLPI2C_TransferDefaultFlag;

    status = LPI2C_MasterTransferBlocking(SHT40_LPI2C_BASE, &xfer);
    if (status != kStatus_Success)
    {
        return false;
    }

    /* Datasheet: high-repeatability conversion takes up to 8.3ms. */
    SDK_DelayAtLeastUs(10000U, SystemCoreClock);

    xfer.direction = kLPI2C_Read;
    xfer.data      = raw;
    xfer.dataSize  = sizeof(raw);

    status = LPI2C_MasterTransferBlocking(SHT40_LPI2C_BASE, &xfer);
    if (status != kStatus_Success)
    {
        return false;
    }

    if (SHT40_Crc8(&raw[0], 2) != raw[2] || SHT40_Crc8(&raw[3], 2) != raw[5])
    {
        return false;
    }

    uint16_t rawTemp = ((uint16_t)raw[0] << 8) | raw[1];
    uint16_t rawHum  = ((uint16_t)raw[3] << 8) | raw[4];

    *tempC       = -45.0f + 175.0f * ((float)rawTemp / 65535.0f);
    *humidityPct = -6.0f  + 125.0f * ((float)rawHum  / 65535.0f);
    if (*humidityPct < 0.0f)   { *humidityPct = 0.0f; }
    if (*humidityPct > 100.0f) { *humidityPct = 100.0f; }

    return true;
}
