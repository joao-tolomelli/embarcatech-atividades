#include "mqtt.h"

//===============================
// Configurações MQTT
//===============================
#define MQTT_BROKER        "192.168.5.196"  
#define MQTT_BROKER_PORT   1883
#define MQTT_CLIENT_ID     "pico_freertos_client"
#define MQTT_KEEPALIVE     60

//===============================
// Variáveis Globais
//===============================
static mqtt_client_t *mqtt_client;
static ip_addr_t broker_ip;
static bool mqtt_connected = false;

static QueueHandle_t mqttQueue = NULL;

// Forward declarations
static void mqtt_task(void *pv);
static void mqtt_dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);


//=========================================================
//                Inicialização do MQTT
//=========================================================
void mqtt_start() {
    mqtt_client = mqtt_client_new();
    if (!mqtt_client) {
        printf("[MQTT] ERRO: mqtt_client_new() retornou NULL!\n");
        return;
    }

    mqttQueue = xQueueCreate(10, sizeof(mqtt_message_t));
    if (!mqttQueue) {
        printf("[MQTT] ERRO: Falha ao criar fila MQTT!\n");
        return;
    }

    xTaskCreate(mqtt_task, "MQTT_Task", 4096, NULL, 1, NULL);
}

bool mqtt_is_connected() {
    return mqtt_connected;
}


//=========================================================
//           Publicação assíncrona pela Queue
//=========================================================
void mqtt_publish_async(const char *topic, const char *payload) {
    mqtt_message_t msg;

    snprintf(msg.topic, sizeof(msg.topic), "%s", topic);
    snprintf(msg.payload, sizeof(msg.payload), "%s", payload);

    if (mqttQueue != NULL) {
        xQueueSend(mqttQueue, &msg, 0);
    }
}


//=========================================================
//                  DNS Callback
//=========================================================
static void mqtt_dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr == NULL) {
        printf("[MQTT] DNS falhou para %s\n", name);
        return;
    }

    broker_ip = *ipaddr;

    printf("[MQTT] DNS Resolvido: %s -> %s\n",
           name,
           ipaddr_ntoa(ipaddr));

    struct mqtt_connect_client_info_t client_info = {
        .client_id = MQTT_CLIENT_ID,
        .keep_alive = MQTT_KEEPALIVE,
        .client_user = NULL,
        .client_pass = NULL
    };

    printf("[MQTT] Conectando ao broker...\n");

    mqtt_client_connect(
        mqtt_client,
        &broker_ip,
        MQTT_BROKER_PORT,
        mqtt_connection_cb,
        NULL,
        &client_info
    );
}


//=========================================================
//                Callback de conexão MQTT
//=========================================================
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status) {

    if (status == MQTT_CONNECT_ACCEPTED) {
        mqtt_connected = true;
        printf("[MQTT] Conectado ao broker!\n");
    } else {
        mqtt_connected = false;
        printf("[MQTT] ERRO de conexão MQTT (%d)\n", status);
    }
}


//=========================================================
//                   Tarefa Principal MQTT
//=========================================================
static void mqtt_task(void *pv) {

    while (1) {

        // ============================
        // Tentativa de conexão
        // ============================
        if (!mqtt_connected) {
            printf("[MQTT] Resolvendo DNS...\n");

            err_t err = dns_gethostbyname(
                MQTT_BROKER,
                &broker_ip,
                mqtt_dns_callback,
                NULL
            );

            if (err == ERR_OK) {
                mqtt_dns_callback(MQTT_BROKER, &broker_ip, NULL);
            }

            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        // ============================
        // Se conectado → publicar mensagens
        // ============================
        if (mqtt_connected) {

            mqtt_message_t msg;

            // Bloqueia até 1s por mensagem
            if (xQueueReceive(mqttQueue, &msg, pdMS_TO_TICKS(1000))) {

                err_t pub_err = mqtt_publish(
                    mqtt_client,
                    msg.topic,
                    msg.payload,
                    strlen(msg.payload),
                    0, // QoS 0
                    0, // retain
                    NULL,
                    NULL
                );

                if (pub_err == ERR_OK) {
                    printf("[MQTT] Enviado: %s -> %s\n",
                           msg.topic,
                           msg.payload);
                } else {
                    printf("[MQTT] Erro ao publicar: %d\n", pub_err);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}