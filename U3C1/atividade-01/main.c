// main.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "servo.h"
#include "bh1750.h"

// --- Configuração dos Pinos ---
#define SERVO_PIN 2
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1

// --- Mapeamento da Lógica para Rotação Contínua ---
const float MIN_LUX = 0.0f;
const float MAX_LUX = 1000.0f;

// Ponto central de luz onde o servo deve parar
const float CENTRO_LUX = MAX_LUX / 2.0f; // Ex: 500 Lux

// Define uma "zona morta" para a parada ser mais estável.
// Ex: Se a luz estiver entre (500-50) e (500+50), o servo para.
const float DEAD_ZONE_LUX = 50.0f;

int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    bh1750_init(I2C_PORT);
    servo_init(SERVO_PIN);
    
    printf("Sistema iniciado. Controlando velocidade do servo com sensor BH1750...\n");

    while (true) {
        float lux = bh1750_read_lux(I2C_PORT);
        float command_angle = 90.0f; // Comando padrão é PARAR

        if (lux < 0) {
            printf("Erro ao ler o sensor BH1750!\n");
        } else {
            // Garante que o valor de lux esteja dentro da faixa esperada
            if (lux > MAX_LUX) lux = MAX_LUX;
            if (lux < MIN_LUX) lux = MIN_LUX;
            
            // Lógica de controle de velocidade
            if (lux < (CENTRO_LUX - DEAD_ZONE_LUX)) {
                // Luz baixa: Gira em um sentido
                // Mapeia a faixa de luz baixa (0 a 450) para um comando (0 a 90)
                float range = CENTRO_LUX - DEAD_ZONE_LUX;
                command_angle = 90.0f * (1.0f - (lux / range));
                printf("Luz Baixa (%.2f Lux) -> Girando sentido 1 (Comando: %.1f)\n", lux, command_angle);

            } else if (lux > (CENTRO_LUX + DEAD_ZONE_LUX)) {
                // Luz alta: Gira no outro sentido
                // Mapeia a faixa de luz alta (550 a 1000) para um comando (90 a 180)
                float range = MAX_LUX - (CENTRO_LUX + DEAD_ZONE_LUX);
                float value_in_range = lux - (CENTRO_LUX + DEAD_ZONE_LUX);
                command_angle = 90.0f + 90.0f * (value_in_range / range);
                printf("Luz Alta (%.2f Lux) -> Girando sentido 2 (Comando: %.1f)\n", lux, command_angle);

            } else {
                // Luz no centro (zona morta): Para o servo
                command_angle = 90.0f;
                printf("Luz Média (%.2f Lux) -> PARADO (Comando: %.1f)\n", lux, command_angle);
            }
            
            // Garante que o comando final esteja entre 0 e 180
            if (command_angle < 0.0f) command_angle = 0.0f;
            if (command_angle > 180.0f) command_angle = 180.0f;

            servo_set_angle(SERVO_PIN, command_angle);
        }

        sleep_ms(100);
    }

    return 0;
}