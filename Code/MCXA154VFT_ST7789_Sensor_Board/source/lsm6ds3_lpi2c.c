#include "lsm6ds3_lpi2c.h"
#include "fsl_lpi2c.h"
#include "fsl_common.h"

#define LSM6DS3_LPI2C_BASE LPI2C0

#define LSM6DS3_REG_WHO_AM_I  0x0FU
#define LSM6DS3_WHO_AM_I_VAL  0x6AU  /* 0x6A instead for LSM6DS3TR-C -- see note above */

#define LSM6DS3_REG_CTRL2_G   0x11U
#define LSM6DS3_REG_OUTX_L_G  0x22U  /* X/Y/Z L/H auto-increment from here (IF_INC=1 by default) */

/* Sensitivity for the +-2000 dps range CTRL2_G selects below (dps/LSB) */
//#define LSM6DS3_GYRO_SENS_2000DPS 0.070f  //Original

/* CTRL2_G: ODR 104 Hz, FS +-500 dps -> 0b0110_01_00 */
#define LSM6DS3_CTRL2_G_VALUE     0x64U
/* Sensitivity for +-500 dps (dps/LSB) */
#define LSM6DS3_GYRO_SENS_500DPS  0.0175f



static status_t LSM6DS3_ReadRegs(uint8_t reg, uint8_t *buf, size_t len)
{
    lpi2c_master_transfer_t xfer = {0};
    xfer.slaveAddress   = LSM6DS3_I2C_ADDR;
    xfer.direction      = kLPI2C_Read;
    xfer.subaddress     = reg;
    xfer.subaddressSize = 1;
    xfer.data           = buf;
    xfer.dataSize        = len;
    xfer.flags           = kLPI2C_TransferDefaultFlag;
    return LPI2C_MasterTransferBlocking(LSM6DS3_LPI2C_BASE, &xfer);
}

static status_t LSM6DS3_WriteReg(uint8_t reg, uint8_t value)
{
    lpi2c_master_transfer_t xfer = {0};
    xfer.slaveAddress   = LSM6DS3_I2C_ADDR;
    xfer.direction      = kLPI2C_Write;
    xfer.subaddress     = reg;
    xfer.subaddressSize = 1;
    xfer.data           = &value;
    xfer.dataSize        = 1;
    xfer.flags           = kLPI2C_TransferDefaultFlag;
    return LPI2C_MasterTransferBlocking(LSM6DS3_LPI2C_BASE, &xfer);
}

bool LSM6DS3_Init(void)
{
    uint8_t whoAmI = 0;

    if (LSM6DS3_ReadRegs(LSM6DS3_REG_WHO_AM_I, &whoAmI, 1) != kStatus_Success)
    {
        return false;
    }
    if (whoAmI != LSM6DS3_WHO_AM_I_VAL)
    {
        return false;  /* wrong address/variant -- see notes above */
    }

    /* CTRL2_G: ODR 104 Hz (0110), FS +-2000 dps (11) -> 0b0110_11_00 */
    //if (LSM6DS3_WriteReg(LSM6DS3_REG_CTRL2_G, 0x6CU) != kStatus_Success)

    /* CTRL2_G: ODR 104 Hz, FS +-500 dps */
    if (LSM6DS3_WriteReg(LSM6DS3_REG_CTRL2_G, LSM6DS3_CTRL2_G_VALUE) != kStatus_Success)

    {
        return false;
    }

    return true;
}

bool LSM6DS3_ReadGyro(float *gx, float *gy, float *gz)
{
    uint8_t raw[6];

    if (LSM6DS3_ReadRegs(LSM6DS3_REG_OUTX_L_G, raw, sizeof(raw)) != kStatus_Success)
    {
        return false;
    }

    int16_t rawX = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
    int16_t rawY = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
    int16_t rawZ = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);

    *gx = (float)rawX * LSM6DS3_GYRO_SENS_500DPS;
    *gy = (float)rawY * LSM6DS3_GYRO_SENS_500DPS;
    *gz = (float)rawZ * LSM6DS3_GYRO_SENS_500DPS;

    return true;
}
