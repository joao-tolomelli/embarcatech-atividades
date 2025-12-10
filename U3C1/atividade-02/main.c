#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

// Inclusão das bibliotecas fornecidas
#include "aht10.h"
#include "ssd1306.h"

// --- Configurações do Display OLED (i2c1) ---
#define I2C1_PORT i2c1
#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDR 0x3C

// --- Configurações do Sensor AHT10 (i2c0) ---
#define I2C0_PORT i2c0
#define I2C0_SDA_PIN 0
#define I2C0_SCL_PIN 1

// --- Funções Wrapper para a interface do AHT10 ---
int i2c_write_wrapper(uint8_t addr, const uint8_t *data, uint16_t len) {
    int result = i2c_write_blocking(I2C0_PORT, addr, data, len, false);
    // Retorna -1 em caso de erro (retorno negativo), 0 caso contrário.
    return result < 0 ? -1 : 0; 
}

int i2c_read_wrapper(uint8_t addr, uint8_t *data, uint16_t len) {
    int result = i2c_read_blocking(I2C0_PORT, addr, data, len, false);
    // Retorna -1 em caso de erro (retorno negativo), 0 caso contrário.
    return result < 0 ? -1 : 0;
}

void delay_ms_wrapper(uint32_t ms) {
    sleep_ms(ms);
}


int main() {
    // Inicializa a saída padrão (para depuração via USB)
    stdio_init_all();

    // Aguarda um pouco para o monitor serial conectar
    sleep_ms(2000);
    printf("Inicializando monitor de Temp/Umidade...\n");

    // --- Inicialização do I2C0 para o Sensor AHT10 ---
    i2c_init(I2C0_PORT, 100 * 1000); // 100kHz
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
    bi_decl(bi_2pins_with_func(I2C0_SDA_PIN, I2C0_SCL_PIN, GPIO_FUNC_I2C));
    printf("I2C0 (Sensor) inicializado.\n");

    // --- Inicialização do I2C1 para o Display OLED ---
    i2c_init(I2C1_PORT, 400 * 1000); // 400kHz
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);
    bi_decl(bi_2pins_with_func(I2C1_SDA_PIN, I2C1_SCL_PIN, GPIO_FUNC_I2C));
    printf("I2C1 (Display) inicializado.\n");

    // --- Inicialização do Sensor AHT10 ---
    AHT10_Handle aht10;
    aht10.iface.i2c_write = i2c_write_wrapper;
    aht10.iface.i2c_read = i2c_read_wrapper;
    aht10.iface.delay_ms = delay_ms_wrapper;

    if (!AHT10_Init(&aht10)) {
        printf("Falha ao inicializar o sensor AHT10!\n");
        // Mostra o erro no display também
        ssd1306_t disp_err;
        disp_err.external_vcc = false;
        ssd1306_init(&disp_err, OLED_WIDTH, OLED_HEIGHT, OLED_ADDR, I2C1_PORT);
        ssd1306_clear(&disp_err);
        ssd1306_draw_string(&disp_err, 0, 16, 1, "Erro no Sensor");
        ssd1306_draw_string(&disp_err, 0, 32, 1, "AHT10!");
        ssd1306_show(&disp_err);
        while (1); // Trava se o sensor falhar
    }
    printf("Sensor AHT10 inicializado com sucesso.\n");

    // --- Inicialização do Display OLED ---
    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, OLED_WIDTH, OLED_HEIGHT, OLED_ADDR, I2C1_PORT);
    ssd1306_clear(&disp);
    
    ssd1306_draw_string(&disp, 0, 0, 1, "Monitor de");
    ssd1306_draw_string(&disp, 0, 16, 1, "Ambiente");
    ssd1306_show(&disp);
    sleep_ms(2000);

    // Loop principal
    while (true) {
        float temperature = 0.0f;
        float humidity = 0.0f;

        // Tenta ler os dados do sensor
        if (AHT10_ReadTemperatureHumidity(&aht10, &temperature, &humidity)) {
            printf("Temperatura: %.1f C, Umidade: %.1f %%\n", temperature, humidity);

            // Limpa o buffer do display
            ssd1306_clear(&disp);

            // Prepara as strings para exibição
            char temp_str[20];
            char hum_str[20];
            snprintf(temp_str, sizeof(temp_str), "Temp: %.1f C", temperature);
            snprintf(hum_str, sizeof(hum_str), "Umid: %.1f %%", humidity);

            // Desenha as strings no buffer
            ssd1306_draw_string(&disp, 0, 0, 2, temp_str);  // Escala 2 para texto maior
            ssd1306_draw_string(&disp, 0, 24, 2, hum_str); // Posição Y = 24

            // --- Lógica de Alerta ---
            if (humidity > 70.0f || temperature < 20.0f) {
                ssd1306_draw_string(&disp, 15, 52, 1, "!!! ALERTA !!!");
            }
            
            // Envia o buffer para o display
            ssd1306_show(&disp);

        } else {
            printf("Falha ao ler dados do sensor.\n");
            ssd1306_clear(&disp);
            ssd1306_draw_string(&disp, 0, 0, 1, "Erro na leitura!");
            ssd1306_show(&disp);
        }

        // Aguarda 2 segundos antes da próxima leitura
        sleep_ms(2000);
    }

    return 0;
}
