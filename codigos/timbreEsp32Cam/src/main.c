#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Incluimos nuestro archivo de cabecera
#include "funciones.h"

void app_main(void) {
    printf("=======================================\n");
    printf("  INICIANDO PRUEBA DE COMPILACION...   \n");
    printf("=======================================\n");

    // Llamamos a la función que vive en funciones.c
    test_compilacion();

    printf("¡Todo listo para empezar a programar la logica del timbre!\n");

    // Loop infinito básico de FreeRTOS para que el procesador no se reinicie
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}