#ifndef SHT40_LPI2C_H_
#define SHT40_LPI2C_H_

#include <stdint.h>
#include <stdbool.h>

#define SHT40_I2C_ADDR  0x46U

/* Call once at startup, after BOARD_InitBootPins(). Configures LPI2C0. */
void SHT40_Init(void);

/* Blocking single-shot high-precision read (~10ms). Returns false on I2C
 * transfer failure or CRC mismatch -- caller should treat outputs as
 * invalid in that case. */
bool SHT40_ReadMeasurement(float *tempC, float *humidityPct);

#endif /* SHT40_LPI2C_H_ */
