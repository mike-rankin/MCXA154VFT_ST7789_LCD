#ifndef LSM6DS3_LPI2C_H_
#define LSM6DS3_LPI2C_H_

#include <stdint.h>
#include <stdbool.h>

#define LSM6DS3_I2C_ADDR  0x6AU

/* Call once at startup, AFTER SHT40_Init() (or equivalent) has already
 * brought up LPI2C0 -- this only configures the sensor's own registers,
 * it does not touch the I2C peripheral clock/init since it shares the
 * same bus as the SHT40. Returns false if WHO_AM_I doesn't match (wrong
 * address, wiring issue, or wrong part variant -- see note above). */
bool LSM6DS3_Init(void);

/* Blocking read of gyro X/Y/Z in degrees/sec. Returns false on I2C failure. */
bool LSM6DS3_ReadGyro(float *gx, float *gy, float *gz);

#endif /* LSM6DS3_LPI2C_H_ */
