#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

// Incluimos ambos módulos
#include "conexion.h"
#include "funciones.h"

void app_main(void) {
    printf("==================================\n");
    printf("   TIMBRE IOT - ARQUITECTURA 2.0  \n");
    printf("==================================\n");

    // 1. Iniciar NVS (Obligatorio para WiFi y Credenciales)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Iniciar Red (Evalúa si hay WiFi guardado o abre el portal)
    init_wifi();

    // 3. Iniciar Mecatrónica
    init_hardware();

    // 4. Lanzar tareas de hardware
    xTaskCreate(tarea_timbre, "Tarea_Timbre", 2048, NULL, 10, NULL);
}