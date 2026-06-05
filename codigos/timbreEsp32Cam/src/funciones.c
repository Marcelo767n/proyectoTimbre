#include "funciones.h"
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// --- VARIABLES GLOBALES ---
static const char *TAG = "WIFI_TIMBRE";
static int reintentos = 0;
QueueHandle_t cola_timbre = NULL;

// ====================================================================
// 1. MODULO RED (WIFI)
// ====================================================================

static void manejador_eventos_wifi(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Intentando conectar al WiFi...");
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (reintentos < MAX_REINTENTOS) {
            esp_wifi_connect();
            reintentos++;
            ESP_LOGW(TAG, "Fallo conexion. Reintentando... (%d/%d)", reintentos, MAX_REINTENTOS);
        } else {
            ESP_LOGE(TAG, "No se pudo conectar al WiFi.");
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "¡Conexion Exitosa! IP asignada: " IPSTR, IP2STR(&event->ip_info.ip));
        reintentos = 0;
    }
}

void init_wifi(void) {
    ESP_LOGI(TAG, "Inicializando modulo WiFi...");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &manejador_eventos_wifi, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &manejador_eventos_wifi, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

// ====================================================================
// 2. MODULO HARDWARE (BOTON Y RELE)
// ====================================================================

// Función de interrupción de hardware
static void IRAM_ATTR boton_isr_handler(void* arg) {
    uint32_t pin = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(cola_timbre, &pin, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void init_hardware(void) {
    gpio_config_t io_conf = {};

    // Configurar Relé (Salida)
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RELE);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(PIN_RELE, 0); // Arranca apagado

    // Configurar Botón (Entrada con Pull-Up Interno)
    io_conf.intr_type = GPIO_INTR_NEGEDGE; // Dispara al bajar a GND
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1; // Resistencia activada
    gpio_config(&io_conf);

    // Crear cola e interrupción
    cola_timbre = xQueueCreate(5, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BOTON, boton_isr_handler, (void*) PIN_BOTON);
    
    ESP_LOGI("HARDWARE", "Hardware inicializado correctamente.");
}

void tarea_timbre(void *pvParameters) {
    uint32_t pin_pulsado;
    while(1) {
        if(xQueueReceive(cola_timbre, &pin_pulsado, portMAX_DELAY)) {
            ESP_LOGW("HARDWARE", "¡Boton presionado en GPIO %lu!", pin_pulsado);
            
            // Sonar timbre
            gpio_set_level(PIN_RELE, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(PIN_RELE, 0);
            
            ESP_LOGI("HARDWARE", "Campana finalizada. Listo para foto.");
        }
    }
}