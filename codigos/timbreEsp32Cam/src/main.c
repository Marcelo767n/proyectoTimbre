#include <stdio.h>
#include "funciones.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

void app_main(void) {
    printf("==================================\n");
    printf("   TIMBRE IOT - INICIO SISTEMA    \n");
    printf("==================================\n");

    // 1. Inicializar la memoria Flash (NVS) - ¡Obligatorio para WiFi!
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Levantar la conexión WiFi
    init_wifi();

    // 3. Inicializar hardware (Botón y Relé)
    init_hardware();

    // 4. Lanzar tarea del timbre (Esperar pulsación)
    xTaskCreate(tarea_timbre, "Tarea_Timbre", 2048, NULL, 10, NULL);
}