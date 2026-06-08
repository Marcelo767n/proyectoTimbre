#include "funciones.h"
#include "esp_camera.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "TIMBRE_HARDWARE";

// Instancia de la cola definida en el .h
QueueHandle_t cola_timbre = NULL;

// ==========================================
// INICIALIZACIÓN DE HARDWARE (Botón y Relé)
// ==========================================
void init_hardware(void) {
    ESP_LOGI(TAG, "Inicializando hardware mecatrónico...");
    
    // Crear la cola de FreeRTOS para la comunicación de eventos (10 espacios)
    cola_timbre = xQueueCreate(10, sizeof(uint32_t));
    
    // Configurar el pin del Botón como entrada
    gpio_config_t conf_boton = {
        .pin_bit_mask = (1ULL << PIN_BOTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE, 
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf_boton);

    // Configurar el pin del Relé como salida
    gpio_config_t conf_rele = {
        .pin_bit_mask = (1ULL << PIN_RELE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf_rele);
    gpio_set_level(PIN_RELE, 0); // Asegurar que arranque apagado

    // Configurar el Flash
    gpio_set_direction(PIN_FLASH, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_FLASH, 0);
}

// ==========================================
// INICIALIZACIÓN DE LA CÁMARA (AI-Thinker)
// ==========================================
void init_camara(void) {
    ESP_LOGI(TAG, "Inicializando módulo de cámara...");
    
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CAM_PIN_Y2;
    config.pin_d1 = CAM_PIN_Y3;
    config.pin_d2 = CAM_PIN_Y4;
    config.pin_d3 = CAM_PIN_Y5;
    config.pin_d4 = CAM_PIN_Y6;
    config.pin_d5 = CAM_PIN_Y7;
    config.pin_d6 = CAM_PIN_Y8;
    config.pin_d7 = CAM_PIN_Y9;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG; 

    // Configuración segura y rápida para la red
    config.frame_size = FRAMESIZE_VGA; 
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Fallo al iniciar cámara: 0x%x", err);
    }
}

// ==========================================
// ENVÍO HTTP A LA RASPBERRY PI
// ==========================================
void enviar_foto_servidor(camera_fb_t *fb) {
    ESP_LOGI(TAG, "Enviando imagen a Raspberry Pi...");
    
    esp_http_client_config_t config = {
        // IP DEL POCO F5 ACTUALIZADA AQUÍ
        .url = "http://10.241.61.199:8000/upload?id=Timbre_Principal", 
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = true, 
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field(client, (const char *)fb->buf, fb->len);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ Imagen enviada. Código HTTP: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "❌ Error HTTP: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

// ==========================================
// TAREA PRINCIPAL (Cerebro del Sistema)
// ==========================================
void tarea_timbre(void *pvParameters) {
    uint32_t pin_evento;

    while (1) {
        // Escucha tanto el botón físico como la simulación automática de main.c
        if (gpio_get_level(PIN_BOTON) == 1 || xQueueReceive(cola_timbre, &pin_evento, 0) == pdTRUE) {
            ESP_LOGI(TAG, "¡Alarma disparada! Iniciando captura...");

            gpio_set_level(PIN_FLASH, 1);
            vTaskDelay(pdMS_TO_TICKS(500)); // Estabilizar luz

            camera_fb_t *fb = esp_camera_fb_get();
            if (fb) {
                gpio_set_level(PIN_FLASH, 0); // Apagar inmediatamente
                enviar_foto_servidor(fb);
                esp_camera_fb_return(fb); // Liberar RAM (Vital)
            } else {
                ESP_LOGE(TAG, "Fallo al capturar imagen");
                gpio_set_level(PIN_FLASH, 0);
            }

            ESP_LOGI(TAG, "Entrando en recarga térmica (5 segundos)...");
            vTaskDelay(pdMS_TO_TICKS(5000)); 
        }
        
        vTaskDelay(pdMS_TO_TICKS(100)); // Evitar saturar el procesador
    }
}