#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/cyw43_arch.h"

// FreeRTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// Imports externos
#include "ssd1306.h"
#include "wifi.h"
#include "mqtt.h"

// I2C configuratção (usando sua escolha)
#define I2C_PORT i2c1
#define SDA_PIN 14
#define SCL_PIN 15

// Protótipos das tasks
void display_task(void *pv);
void mqtt_task(void *pv);

int main()
{
    stdio_init_all();

    if (cyw43_arch_init())
    {
        printf("Erro ao iniciar CYW43\n");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    // Inicia o WiFi
    wifi_init();

    // Inicia o MQTT
    mqtt_start();

    // Configura I2C1
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    // Cria tasks
    xTaskCreate(mqtt_task, "mqtt", 4096, NULL, 2, NULL);
    xTaskCreate(display_task, "display", 3072, &disp, 1, NULL);

    // inicia FreeRTOS
    vTaskStartScheduler();

    while (true)
    {
        // Nunca deve chegar aqui
    };
}

void display_task(void *pv)
{
    // Recebe o ponteiro para a estrutura do display
    ssd1306_t *disp = (ssd1306_t *)pv;

    while (true)
    {
        ssd1306_clear(disp);
        ssd1306_draw_string(disp, 16, 55, 1, "FreeRTOS + MQTT");
        ssd1306_show(disp);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Atualiza a cada 1 segundo
    }
}

void mqtt_task(void *pv)
{
    while (1)
    {
        if (mqtt_is_connected())
        {
            // Publica mensagem de teste
            mqtt_publish_async("test/topic", "Hello from FreeRTOS on Pico!");
        }
        else
        {
            // Tenta reconectar se não estiver conectado
            printf("Tentando reconectar ao MQTT...\n");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
