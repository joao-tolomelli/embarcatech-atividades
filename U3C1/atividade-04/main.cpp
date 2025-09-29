#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Inclui as bibliotecas em C dentro de um bloco 'extern "C"'
extern "C" {
#include "servo.h"
#include "ssd1306.h"
}

// Inclui a biblioteca do MPU6050 em C++.
#include "MPU6050.h"

// --- CONSTANTES DE CONFIGURAÇÃO ---

// --- Configurações do Display OLED (i2c1) ---
#define I2C1_PORT i2c1
#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

// --- Configurações do Sensor MPU6050 (i2c0) ---
#define I2C0_PORT i2c0
#define I2C0_SDA_PIN 0
#define I2C0_SCL_PIN 1
#define MPU6050_ADDR 0x68

const float ACCEL_SCALE_FACTOR = 16384.0f;

// --- Configuração do Servo ---
#define SERVO_PIN 2

// Velocidade para as duas portas I2C
#define I2C_SPEED 400 * 1000

// Constantes para o servo de rotação contínua
const float ANGLE_ALERT_THRESHOLD = 45.0f;
#define SERVO_STOP_PULSE_US 1500
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2500
#define SERVO_DEAD_ZONE_DEGREES 5.0f

float map_value(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
    return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

// --- Função Principal ---
int main() {
    stdio_init_all();
    sleep_ms(2000);

    printf("Iniciando sistema com portas I2C e Servo atualizadas\n");

    // --- Inicialização da Porta I2C0 para o MPU6050 ---
    i2c_init(I2C0_PORT, I2C_SPEED);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
    printf("I2C0 (MPU6050) inicializado nos pinos SDA=%d, SCL=%d\n", I2C0_SDA_PIN, I2C0_SCL_PIN);

    // --- Inicialização da Porta I2C1 para o Display OLED ---
    i2c_init(I2C1_PORT, I2C_SPEED);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);
    printf("I2C1 (OLED) inicializado nos pinos SDA=%d, SCL=%d\n", I2C1_SDA_PIN, I2C1_SCL_PIN);

    // Inicializa o MPU6050 na porta I2C0
    MPU6050 mpu(I2C0_PORT, MPU6050_ADDR);
    mpu.begin();
    printf("MPU6050 inicializado. ID: 0x%X\n", mpu.getId());
    
    // Inicializa o Servo no novo pino
    servo_init(SERVO_PIN);
    printf("Servo motor inicializado no pino GPIO%d.\n", SERVO_PIN);

    // Inicializa o Display OLED na porta I2C1
    ssd1306_t disp;
    disp.external_vcc = false; // <-- CORREÇÃO APLICADA AQUI
    ssd1306_init(&disp, OLED_WIDTH, OLED_HEIGHT, OLED_ADDR, I2C1_PORT);
    printf("Display OLED SSD1306 inicializado.\n");
    
    MPU6050::VECT_3D accel;
    float pitch;
    uint32_t pulse_width_us;

    while (1) {
        mpu.getAccel(&accel, ACCEL_SCALE_FACTOR);
        pitch = atan2(-accel.x, sqrt(accel.y * accel.y + accel.z * accel.z)) * 180.0 / M_PI;
        
        printf("Angulo (Pitch): %.2f graus | ", pitch);

        if (fabs(pitch) < SERVO_DEAD_ZONE_DEGREES) {
            pulse_width_us = SERVO_STOP_PULSE_US;
        } else {
            pulse_width_us = (uint32_t)map_value(pitch, -90.0f, 90.0f, 
                                                  SERVO_MAX_PULSE_US, SERVO_MIN_PULSE_US);

            if (pulse_width_us < SERVO_MIN_PULSE_US) pulse_width_us = SERVO_MIN_PULSE_US;
            if (pulse_width_us > SERVO_MAX_PULSE_US) pulse_width_us = SERVO_MAX_PULSE_US;
        }

        servo_set_pulse_width(SERVO_PIN, pulse_width_us);
        printf("Largura do Pulso: %u us\n", pulse_width_us);

        ssd1306_clear(&disp);
        char display_buffer[20];
        if (fabs(pitch) > ANGLE_ALERT_THRESHOLD) {
            ssd1306_draw_string(&disp, 25, 16, 2, "ALERTA!");
            snprintf(display_buffer, sizeof(display_buffer), "%.1f graus", pitch);
            ssd1306_draw_string(&disp, 15, 40, 1, display_buffer);
        } else {
            ssd1306_draw_string(&disp, 0, 16, 1, "Velocidade Servo:");
            snprintf(display_buffer, sizeof(display_buffer), "%u us", pulse_width_us);
            ssd1306_draw_string(&disp, 20, 35, 2, display_buffer);
        }
        ssd1306_show(&disp);

        sleep_ms(100);
    }

    return 0;
}