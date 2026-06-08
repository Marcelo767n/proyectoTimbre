#include "esp_camera.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// --- CONFIGURACIÓN DE PINES ---
#define PIN_BOTON GPIO_NUM_12 // Pin donde conectarás el botón (con su resistencia pull-up/down)
#define PIN_FLASH GPIO_NUM_4  // Pin del LED Flash de la ESP32-CAM

static const char *TAG = "TIMBRE_HARDWARE";

// Función para enviar la foto al servidor Edge
void enviar_foto_servidor(camera_fb_t *fb) {
    ESP_LOGI(TAG, "Conectando al servidor Edge local...");
    
    esp_http_client_config_t config = {
        // IP local de la Raspberry + Puerto 8000 + ID del dispositivo
        .url = "http://192.168.1.20:8000/upload?id=Timbre_Principal", 
        .method = HTTP_METHOD_POST,
        .skip_cert_common_name_check = true, 
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field(client, (const char *)fb->buf, fb->len);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Imagen enviada con éxito. Código HTTP: %d", esp_http_client_get_status_code(client));
    } else {
        ESP_LOGE(TAG, "Error al enviar la imagen: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
}

// Tarea principal de monitoreo del hardware
void tarea_timbre(void *pvParameters) {
    // Configurar el pin del botón como entrada
    gpio_set_direction(PIN_BOTON, GPIO_MODE_INPUT);
    // Configurar el pin del flash como salida
    gpio_set_direction(PIN_FLASH, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_FLASH, 0); // Asegurar que el flash empiece apagado

    while (1) {
        // Leer el estado del botón (asumiendo lógica Pull-Down: 1 es presionado)
        // El capacitor de 100nF que compraste ayudará con el debounce físico aquí
        if (gpio_get_level(PIN_BOTON) == 1) {
            ESP_LOGI(TAG, "¡Botón presionado! Iniciando secuencia de captura...");

            // 1. Encender el flash para iluminar al visitante
            gpio_set_level(PIN_FLASH, 1);
            vTaskDelay(pdMS_TO_TICKS(500)); // Esperar medio segundo para estabilizar la luz

            // 2. Tomar la fotografía
            camera_fb_t *fb = esp_camera_fb_get();
            if (!fb) {
                ESP_LOGE(TAG, "Fallo al capturar la imagen");
                gpio_set_level(PIN_FLASH, 0);
                continue;
            }

            // 3. Apagar el flash inmediatamente después de la foto
            gpio_set_level(PIN_FLASH, 0);

            // 4. Enviar la foto a la Raspberry Pi
            enviar_foto_servidor(fb);

            // 5. Liberar la memoria de la cámara (VITAL para no colapsar la RAM)
            esp_camera_fb_return(fb);

            // 6. Pausa de seguridad (Cooldown) para evitar spam si mantienen el botón presionado
            ESP_LOGI(TAG, "Entrando en tiempo de recarga (5 segundos)...");
            vTaskDelay(pdMS_TO_TICKS(5000)); 
        }
        
        // Pequeño delay para no saturar el procesador (Watchdog)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}