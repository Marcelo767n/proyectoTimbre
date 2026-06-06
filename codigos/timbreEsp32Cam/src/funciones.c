#include "funciones.h"
#include <stdio.h>
#include "esp_log.h"
#include "esp_http_client.h"


QueueHandle_t cola_timbre = NULL;

// Interrupción del botón
static void IRAM_ATTR boton_isr_handler(void* arg) {
    uint32_t pin = (uint32_t) arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(cola_timbre, &pin, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

// Inicialización de Botón y Relé
void init_hardware(void) {
    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RELE);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(PIN_RELE, 0);

    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    cola_timbre = xQueueCreate(5, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BOTON, boton_isr_handler, (void*) PIN_BOTON);
    // Flash LED
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_FLASH);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(PIN_FLASH, 0); // Nos aseguramos de que inicie apagado
    
    ESP_LOGI("HARDWARE", "Controles mecatrónicos listos.");
}

// Inicialización de la Cámara
void init_camara(void) {
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
    
    // Configuración de calidad y tamaño
    config.frame_size = FRAMESIZE_VGA; // 640x480
    config.jpeg_quality = 12; // Menor número = mayor calidad (0-63)
    config.fb_count = 1; // 1 imagen en el Frame Buffer

    // Inicializar el sensor
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE("CAMARA", "Error al iniciar la camara: 0x%x", err);
        return;
    }
    ESP_LOGI("CAMARA", "Sensor OV2640 inicializado correctamente.");
}

// Tarea principal (Botón -> Relé -> Foto)
void tarea_timbre(void *pvParameters) {
    uint32_t pin_pulsado;
    while(1) {
        if(xQueueReceive(cola_timbre, &pin_pulsado, portMAX_DELAY)) {
            ESP_LOGW("SISTEMA", "¡Timbre presionado!");
            
            // 1. Suena el timbre
            gpio_set_level(PIN_RELE, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(PIN_RELE, 0);
            
            // 2. Encender Flash y dar tiempo de ajuste al lente
            ESP_LOGW("CAMARA", "Encendiendo Flash...");
            gpio_set_level(PIN_FLASH, 1);
            
            // Esperamos 800ms para que el sensor ajuste el brillo automáticamente
            vTaskDelay(pdMS_TO_TICKS(800)); 
            
            // 3. Tomar la foto iluminada
            ESP_LOGI("CAMARA", "Capturando imagen...");
            camera_fb_t * pic = esp_camera_fb_get();
            
            // Apagamos el flash inmediatamente para no gastar energía ni generar calor
            gpio_set_level(PIN_FLASH, 0);
            ESP_LOGW("CAMARA", "Flash apagado.");
            
            if(!pic) {
                ESP_LOGE("CAMARA", "Fallo al capturar la imagen. Frame buffer vacío.");
            } else {
                ESP_LOGI("CAMARA", "¡Éxito! Foto tomada. Tamaño en RAM: %zu bytes", pic->len);
                
                // --- INICIO DEL ENVÍO HTTP ---
                ESP_LOGI("HTTP", "Conectando al servidor...");
                esp_http_client_config_t config = {
                    .url = "http://192.168.1.5:8000/upload", 
                    .method = HTTP_METHOD_POST,
                };
                esp_http_client_handle_t client = esp_http_client_init(&config);
                
                esp_http_client_set_header(client, "Content-Type", "image/jpeg");
                esp_http_client_set_post_field(client, (const char *)pic->buf, pic->len);
                
                esp_err_t err = esp_http_client_perform(client);
                if (err == ESP_OK) {
                    ESP_LOGI("HTTP", "¡Foto entregada al servidor! Status = %d", esp_http_client_get_status_code(client));
                } else {
                    ESP_LOGE("HTTP", "Error al enviar: %s", esp_err_to_name(err));
                }
                esp_http_client_cleanup(client);
                // --- FIN DEL ENVÍO HTTP ---
                
                esp_camera_fb_return(pic);
            }
        }
    }
}