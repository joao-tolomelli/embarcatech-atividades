#include <stdio.h>
#include "pico/cyw43_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "wifi.h"

#define WIFI_SSID      "Tolomelli TW"
#define WIFI_PASSWORD  "JvGa@2728"

static SemaphoreHandle_t xWiFiMutex;
static bool wifi_connected = false;

static void vTaskWiFiManager(void *pv) {
    struct netif *netif = &cyw43_state.netif[0];

    while (1) {
        if (xSemaphoreTake(xWiFiMutex, portMAX_DELAY)) {
            if (!wifi_connected || netif->ip_addr.addr == 0) {
                printf("Tentando conectar em '%s'...\n", WIFI_SSID);

                int r = cyw43_arch_wifi_connect_timeout_ms(
                    WIFI_SSID,
                    WIFI_PASSWORD,
                    CYW43_AUTH_WPA2_AES_PSK,
                    15000
                );

                if (r == 0) {
                    wifi_connected = true;
                    printf("Conectado! IP: %s\n", ip4addr_ntoa(&netif->ip_addr));
                } else {
                    wifi_connected = false;
                    printf("Falha ao conectar.\n");
                }
            }

            xSemaphoreGive(xWiFiMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void wifi_init(void) {
    xWiFiMutex = xSemaphoreCreateMutex();
    xTaskCreate(vTaskWiFiManager, "WiFiManager", 2048, NULL, 1, NULL);
}