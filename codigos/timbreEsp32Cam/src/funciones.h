#ifndef FUNCIONES_H
#define FUNCIONES_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_camera.h"

// --- PINES DEL HARDWARE EXTERNO ---
#define PIN_BOTON GPIO_NUM_15
#define PIN_RELE  GPIO_NUM_13
#define PIN_FLASH GPIO_NUM_4

// --- PINES INTERNOS DE LA CAMARA (AI-THINKER) ---
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_Y9 35
#define CAM_PIN_Y8 34
#define CAM_PIN_Y7 39
#define CAM_PIN_Y6 36
#define CAM_PIN_Y5 21
#define CAM_PIN_Y4 19
#define CAM_PIN_Y3 18
#define CAM_PIN_Y2 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

extern QueueHandle_t cola_timbre;

void init_hardware(void);
void init_camara(void);
void tarea_timbre(void *pvParameters);

#endif