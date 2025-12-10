#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include "lwip/ip_addr.h"
#include "lwip/dns.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

typedef struct {
    char topic[128];
    char payload[256];
} mqtt_message_t;

void mqtt_start();
bool mqtt_is_connected();
void mqtt_publish_async(const char *topic, const char *payload);

#endif