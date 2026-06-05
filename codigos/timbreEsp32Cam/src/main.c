#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "conexion.h"
#include "funciones.h"

void app_main(void) {
    printf("==================================\n");
    printf("   TIMBRE IOT - ARQUITECTURA 2.0  \n");
    printf("==================================\n");

    // 1. Iniciar NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Iniciar Red
    init_wifi();

    // 3. Iniciar Mecatrónica y Visión
    init_hardware();
    init_camara();

    // 4. Lanzar tareas
    xTaskCreate(tarea_timbre, "Tarea_Timbre", 4096, NULL, 10, NULL);

    // ==========================================
    // SIMULACIÓN DE HARDWARE (TEST AUTOMÁTICO)
    // ==========================================
    ESP_LOGW("TEST", "Esperando 8 segundos para el auto-disparo...");
    vTaskDelay(pdMS_TO_TICKS(8000)); 
    
    uint32_t pin_simulado = PIN_BOTON;
    // Inyectamos la señal virtual directo al cerebro de la tarea
    xQueueSend(cola_timbre, &pin_simulado, portMAX_DELAY);
}