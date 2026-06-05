#ifndef FUNCIONES_H
#define FUNCIONES_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Pines
#define PIN_BOTON GPIO_NUM_15
#define PIN_RELE  GPIO_NUM_13

extern QueueHandle_t cola_timbre;

void init_hardware(void);
void tarea_timbre(void *pvParameters);

#endif