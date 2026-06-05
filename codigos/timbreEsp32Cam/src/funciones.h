#ifndef FUNCIONES_H
#define FUNCIONES_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// --- CREDENCIALES WIFI ---
#define WIFI_SSID      "Flia. NAVARRO"
#define WIFI_PASS      "Navarro711"
#define MAX_REINTENTOS 5

// --- PINES HARDWARE ---
#define PIN_BOTON GPIO_NUM_15
#define PIN_RELE  GPIO_NUM_13

extern QueueHandle_t cola_timbre;

// --- PROTOTIPOS ---
void init_hardware(void);
void tarea_timbre(void *pvParameters);
void init_wifi(void);

#endif