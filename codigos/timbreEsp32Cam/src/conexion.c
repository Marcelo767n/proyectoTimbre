#include "conexion.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_http_server.h"
#include "lwip/sockets.h" // Librería crítica para el DNS

#define MAX_REINTENTOS 5
static const char *TAG = "WIFI_TIMBRE";
static int reintentos = 0;
static httpd_handle_t servidor_web = NULL;

// ====================================================================
// 1. SERVIDOR WEB Y PORTAL CAUTIVO
// ====================================================================

// La página web que verá el usuario
static esp_err_t html_handler(httpd_req_t *req) {
    const char* html_page = 
        "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<style>body{font-family:Arial;text-align:center;margin-top:50px;} input{padding:10px;margin:10px;font-size:16px;} button{padding:10px 20px;background-color:#4CAF50;color:white;border:none;cursor:pointer;font-size:16px;}</style></head>"
        "<body><h2>Configura tu Timbre IoT</h2>"
        "<form action=\"/guardar\" method=\"post\">"
        "<input type=\"text\" name=\"ssid\" placeholder=\"Nombre de tu WiFi\"><br>"
        "<input type=\"password\" name=\"pass\" placeholder=\"Contrasena\"><br>"
        "<button type=\"submit\">Conectar Timbre</button>"
        "</form></body></html>";
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Guarda los datos ingresados en la memoria NVS y reinicia
static esp_err_t guardar_credenciales_handler(httpd_req_t *req) {
    char buf[100];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) return ESP_FAIL;
    buf[len] = '\0';

    char ssid[32] = {0};
    char pass[64] = {0};
    
    char *ssid_start = strstr(buf, "ssid=");
    char *pass_start = strstr(buf, "pass=");
    
    if (ssid_start && pass_start) {
        ssid_start += 5;
        char *ssid_end = strchr(ssid_start, '&');
        if (ssid_end) {
            strncpy(ssid, ssid_start, ssid_end - ssid_start);
            pass_start += 5;
            strncpy(pass, pass_start, strlen(pass_start));

            nvs_handle_t nvs_handle;
            nvs_open("storage", NVS_READWRITE, &nvs_handle);
            nvs_set_str(nvs_handle, "ssid", ssid);
            nvs_set_str(nvs_handle, "pass", pass);
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);

            httpd_resp_send(req, "<h2>¡Datos guardados! El timbre se reiniciara en unos segundos...</h2>", HTTPD_RESP_USE_STRLEN);
            ESP_LOGW(TAG, "Credenciales guardadas. Reiniciando en 2 seg...");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
        }
    }
    return ESP_OK;
}

// Intercepta cualquier búsqueda fallida y la redirige a la IP de la ESP32
static esp_err_t error_404_handler(httpd_req_t *req, httpd_err_code_t err) {
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "http://192.168.4.1/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static void iniciar_servidor_web(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    if (httpd_start(&servidor_web, &config) == ESP_OK) {
        httpd_uri_t uri_get = { .uri = "/", .method = HTTP_GET, .handler = html_handler, .user_ctx = NULL };
        httpd_register_uri_handler(servidor_web, &uri_get);

        httpd_uri_t uri_post = { .uri = "/guardar", .method = HTTP_POST, .handler = guardar_credenciales_handler, .user_ctx = NULL };
        httpd_register_uri_handler(servidor_web, &uri_post);

        // Activamos la trampa de redirección
        httpd_register_err_handler(servidor_web, HTTPD_404_NOT_FOUND, error_404_handler);
    }
}

// ====================================================================
// 2. SERVIDOR DNS FALSO (Engaña al Celular)
// ====================================================================

static void tarea_dns_servidor(void *pvParameters) {
    struct sockaddr_in serv_addr;
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(53); // Puerto DNS estándar
    bind(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    char rx_buffer[128];
    while(1) {
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer), 0, (struct sockaddr *)&source_addr, &socklen);
        
        if (len > 0 && len < 100) {
            // Modificamos el paquete de respuesta diciendo: "Toda web es 192.168.4.1"
            rx_buffer[2] |= 0x80; 
            rx_buffer[3] |= 0x80; 
            rx_buffer[7] = 1;     
            
            char *respuesta = rx_buffer + len;
            *respuesta++ = 0xc0; *respuesta++ = 0x0c; 
            *respuesta++ = 0x00; *respuesta++ = 0x01; 
            *respuesta++ = 0x00; *respuesta++ = 0x01; 
            *respuesta++ = 0x00; *respuesta++ = 0x00; *respuesta++ = 0x00; *respuesta++ = 0x3c; 
            *respuesta++ = 0x00; *respuesta++ = 0x04; 
            *respuesta++ = 192; *respuesta++ = 168; *respuesta++ = 4; *respuesta++ = 1; 
            
            sendto(sock, rx_buffer, len + 16, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ====================================================================
// 3. LOGICA PRINCIPAL DE CONEXION (STA / AP)
// ====================================================================

static void manejador_eventos_wifi(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (reintentos < MAX_REINTENTOS) {
            esp_wifi_connect();
            reintentos++;
            ESP_LOGW(TAG, "Reintentando conexion...");
        } else {
            ESP_LOGE(TAG, "No se pudo conectar. Abriendo Portal (Timbre_Setup).");
            esp_wifi_stop();
            wifi_config_t ap_config = {
                .ap = { .ssid = "Timbre_Setup", .ssid_len = strlen("Timbre_Setup"), .max_connection = 4, .authmode = WIFI_AUTH_OPEN },
            };
            esp_wifi_set_mode(WIFI_MODE_AP);
            esp_wifi_set_config(WIFI_IF_AP, &ap_config);
            esp_wifi_start();
            iniciar_servidor_web();
            xTaskCreate(tarea_dns_servidor, "tarea_dns", 2048, NULL, 5, NULL); // Inicia el engaño DNS
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "¡Conexion Exitosa! IP asignada: " IPSTR, IP2STR(&event->ip_info.ip));
        reintentos = 0;
    }
}

void init_wifi(void) {
    ESP_LOGI(TAG, "Leyendo credenciales en NVS...");
    char saved_ssid[32] = {0};
    char saved_pass[64] = {0};
    size_t ssid_len = sizeof(saved_ssid);
    size_t pass_len = sizeof(saved_pass);

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    bool credentials_found = false;
    
    if (err == ESP_OK) {
        if (nvs_get_str(nvs_handle, "ssid", saved_ssid, &ssid_len) == ESP_OK &&
            nvs_get_str(nvs_handle, "pass", saved_pass, &pass_len) == ESP_OK) {
            credentials_found = true;
            ESP_LOGI(TAG, "Red encontrada: %s", saved_ssid);
        }
        nvs_close(nvs_handle);
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &manejador_eventos_wifi, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &manejador_eventos_wifi, NULL, NULL);

    if (credentials_found) {
        wifi_config_t wifi_config = {};
        strcpy((char*)wifi_config.sta.ssid, saved_ssid);
        strcpy((char*)wifi_config.sta.password, saved_pass);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    } else {
        ESP_LOGW(TAG, "Sin credenciales. Iniciando Portal Cautivo...");
        wifi_config_t ap_config = {
            .ap = { .ssid = "Timbre_Setup", .ssid_len = strlen("Timbre_Setup"), .max_connection = 4, .authmode = WIFI_AUTH_OPEN },
        };
        esp_wifi_set_mode(WIFI_MODE_AP);
        esp_wifi_set_config(WIFI_IF_AP, &ap_config);
        iniciar_servidor_web();
        xTaskCreate(tarea_dns_servidor, "tarea_dns", 2048, NULL, 5, NULL); // Inicia el engaño DNS
    }
    esp_wifi_start();
}