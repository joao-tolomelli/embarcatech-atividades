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
#include "aht10.h"

// Estrutura global para dados dos sensores
typedef struct
{
    float temperatura;
    float umidade;
    float luminosidade;
} sensor_data_t;

volatile sensor_data_t sensor_data;

// Mutex para proteger acesso aos dados dos sensores
SemaphoreHandle_t xSensorMutex;

// Configurações do I2C0
#define I2C0_PORT i2c0
#define I2C0_SDA_PIN 0
#define I2C0_SCL_PIN 1

// Prototipos das funções I2C0
int i2c_write(uint8_t addr, const uint8_t *data, uint16_t len);
int i2c_read(uint8_t addr, uint8_t *data, uint16_t len);
void delay_ms(uint32_t ms);


// Configurações do I2C1
#define I2C1_PORT i2c1
#define I2C1_SDA_PIN 14
#define I2C1_SCL_PIN 15

// Protótipos das tasks
void display_task(void *pv);
void mqtt_task(void *pv);
void aht10_task(void *pv);

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

    // Configura I2C0
    i2c_init(I2C0_PORT, 100000);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);

    // Configura I2C1
    i2c_init(I2C1_PORT, 400000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    // Define estrutura do sensor
    AHT10_Handle aht10 = {
        .iface = {
            .i2c_write = i2c_write,
            .i2c_read = i2c_read,
            .delay_ms = delay_ms
        }
    };

    // Inicializa o sensor AHT10
    printf("Inicializando AHT10...\n");
    if (!AHT10_Init(&aht10)) {
        printf("Falha na inicialização do sensor!\n");
        while (1) sleep_ms(1000);
    }

    // Cria o Mutex
    xSensorMutex = xSemaphoreCreateMutex();
    if (xSensorMutex == NULL)
    {
        printf("Erro ao criar Mutex!\n");
    }

    // Cria tasks
    xTaskCreate(mqtt_task, "mqtt", 4096, NULL, 2, NULL);
    xTaskCreate(display_task, "display", 3072, &disp, 1, NULL);
    xTaskCreate(aht10_task, "AHT10", 2048, &aht10, 3, NULL);

    // inicia FreeRTOS
    vTaskStartScheduler();

    while (true)
    {
        // Nunca deve chegar aqui
    };
}

void aht10_task(void *pv) {
    // 1. Recupera o ponteiro do sensor passado via parâmetro
    AHT10_Handle *sensor = (AHT10_Handle *)pv;

    float temp_lida, hum_lida;

    while (true) {
        // Tenta ler o sensor
        if (AHT10_ReadTemperatureHumidity(sensor, &temp_lida, &hum_lida)) {
            
            // Entra na Seção Crítica (Protege a struct global)
            // Espera até 100ms pelo Mutex se ele estiver ocupado
            if (xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                
                sensor_data.temperatura = temp_lida;
                sensor_data.umidade = hum_lida;
                
                // Libera o Mutex para as outras tasks (MQTT/Display)
                xSemaphoreGive(xSensorMutex);
            }
        } else {
            printf("Erro ao ler AHT10\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void display_task(void *pv)
{
    ssd1306_t *disp = (ssd1306_t *)pv;
    char buffer[32];
    
    // Variáveis locais para armazenar cópia dos dados
    float temp_local = 0.0f;
    float hum_local = 0.0f;

    while (true)
    {
        if (xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            temp_local = sensor_data.temperatura;
            hum_local = sensor_data.umidade;
            xSemaphoreGive(xSensorMutex); 
        }

        ssd1306_clear(disp);
        
        // Formata e exibe usando as variáveis locais
        sprintf(buffer, "Temp: %.1f C", temp_local);
        ssd1306_draw_string(disp, 10, 10, 1, buffer);
        
        sprintf(buffer, "Umid: %.1f %%", hum_local);
        ssd1306_draw_string(disp, 10, 25, 1, buffer);

        ssd1306_show(disp);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void mqtt_task(void *pv)
{
    char payload[100];
    float temp_local;
    float hum_local;

    while (1)
    {
        if (mqtt_is_connected())
        {
            // Entra na Seção Crítica para ler os dados dos sensores
            if (xSemaphoreTake(xSensorMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                // Copia os dados para variáveis locais para liberar o mutex rapidamente
                temp_local = sensor_data.temperatura;
                hum_local = sensor_data.umidade;
                xSemaphoreGive(xSensorMutex);

                // Formata os dados em uma string JSON
                sprintf(payload, "{\"temperatura\": %.1f, \"umidade\": %.1f}", temp_local, hum_local);

                // Publica os dados no tópico MQTT. Você pode alterar "pico/dados".
                mqtt_publish_async("pico/dados", payload);
            }
        }
        else
        {
            // Tenta reconectar se não estiver conectado
            printf("Tentando reconectar ao MQTT...\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Publica a cada 10 segundos
    }
}

// Função para escrita I2C
int i2c_write(uint8_t addr, const uint8_t *data, uint16_t len) {
    int result = i2c_write_blocking(I2C0_PORT, addr, data, len, false);
    return result < 0 ? -1 : 0;
}

// Função para leitura I2C
int i2c_read(uint8_t addr, uint8_t *data, uint16_t len) {
    int result = i2c_read_blocking(I2C0_PORT, addr, data, len, false);
    return result < 0 ? -1 : 0;
}

// Função para delay
void delay_ms(uint32_t ms) {
    sleep_ms(ms);
}
