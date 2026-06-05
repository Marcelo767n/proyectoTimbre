#include "funciones.h"
#include <stdio.h>
#include "esp_log.h"

QueueHandle_t cola_timbre = NULL;

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

    // Relé
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_RELE);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(PIN_RELE, 0);

    // Botón
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BOTON);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    cola_timbre = xQueueCreate(5, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_BOTON, boton_isr_handler, (void*) PIN_BOTON);
    
    ESP_LOGI("HARDWARE", "Hardware de control inicializado.");
}

void tarea_timbre(void *pvParameters) {
    uint32_t pin_pulsado;
    while(1) {
        if(xQueueReceive(cola_timbre, &pin_pulsado, portMAX_DELAY)) {
            ESP_LOGW("HARDWARE", "Boton presionado en GPIO %lu", pin_pulsado);
            
            gpio_set_level(PIN_RELE, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_set_level(PIN_RELE, 0);
            
            ESP_LOGI("HARDWARE", "Timbre sonó. (Aquí dispararemos la cámara)");
        }
    }
}