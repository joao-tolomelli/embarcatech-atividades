#include "mpu_wrapper.h"
#include "MPU6050.h"

// Instância global do objeto C++
MPU6050 *mpu = nullptr;

// Função de inicialização
void mpu6050_init_c(i2c_inst_t *i2c, uint8_t addr) {
    if (mpu == nullptr) {
        mpu = new MPU6050(i2c, addr);
    }
    mpu->begin();
}

// Função de leitura de aceleração
void mpu6050_read_accel_c(float *x, float *y, float *z) {
    if (mpu != nullptr) {
        // --- CORREÇÃO AQUI ---
        // Usamos MPU6050::VECT_3D para dizer que a struct pertence à classe
        MPU6050::VECT_3D v; 
        
        // 16384.0f é a escala padrão para +/- 2g
        mpu->getAccel(&v, 16384.0f);
        
        *x = v.x;
        *y = v.y;
        *z = v.z;
    }
}