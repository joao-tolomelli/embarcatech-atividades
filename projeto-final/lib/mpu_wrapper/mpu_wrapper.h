#ifndef MPU_WRAPPER_H
#define MPU_WRAPPER_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Funções acessíveis pelo C
void mpu6050_init_c(i2c_inst_t *i2c, uint8_t addr);
void mpu6050_read_accel_c(float *x, float *y, float *z);

#ifdef __cplusplus
}
#endif

#endif