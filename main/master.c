// main.c (MAESTRO) - ESP-IDF
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define TAG "MASTER"

// Conexión al AP del esclavo
#define STA_SSID   "LM35_SLAVE_AP"
#define STA_PASS   "12345678"

// UDP
#define UDP_PORT   3333
#define SLAVE_IP   "192.168.4.1"  // IP por defecto del SoftAP del ESP32

static int udp_sock = -1;
static struct sockaddr_in slave_addr;

static volatile float last_temp = NAN;

static void wifi_init_sta(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wc = {0};
    strcpy((char*)wc.sta.ssid, STA_SSID);
    strcpy((char*)wc.sta.password, STA_PASS);
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "Conectando a %s ...", STA_SSID);
}

static void udp_open_master(void){
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_sock < 0){ ESP_LOGE(TAG,"socket failed"); vTaskDelay(portMAX_DELAY); }

    memset(&slave_addr, 0, sizeof(slave_addr));
    slave_addr.sin_family = AF_INET;
    slave_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, SLAVE_IP, &slave_addr.sin_addr);

    // enviar HELLO para que el esclavo recuerde nuestra IP/puerto
    const char *hello = "HELLO\n";
    sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
    ESP_LOGI(TAG, "UDP listo hacia %s:%d", SLAVE_IP, UDP_PORT);
}

// RX de "TEMP:x.y"
static void task_udp_rx_master(void *arg){
    char buf[64];
    struct sockaddr_in src; socklen_t slen=sizeof(src);
    while (1){
        int n = recvfrom(udp_sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&src, &slen);
        if (n>0){
            buf[n]=0;
            if (strncmp(buf,"TEMP:",5)==0){
                last_temp = atof(buf+5);
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

// Muestra cada 5 s
static void task_print_5s(void *arg){
    while (1){
        if (!isnan(last_temp)) printf("[Maestro] Temp actual: %.2f C\n", last_temp);
        else printf("[Maestro] Esperando TEMP...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Consola: "set <valor>"
static void task_console(void *arg){
    char line[64];
    while (1){
        printf("Comando (ej: set 30.5): ");
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin)){
            if (strncmp(line,"set",3)==0){
                float v = atof(line+3);
                char out[32];
                int len = snprintf(out, sizeof(out), "SET:%.2f\n", v);
                sendto(udp_sock, out, len, 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
                // re-envía HELLO por si reinició el esclavo
                const char *hello = "HELLO\n";
                sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
                printf("Setpoint %.2f C enviado.\n", v);
            } else {
                printf("Comando no reconocido. Usa: set <valor>\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    vTaskDelay(pdMS_TO_TICKS(1500)); // margen para DHCP
    udp_open_master();

    xTaskCreate(task_udp_rx_master,   "udp_rx",   3*1024, NULL, 6, NULL);
    xTaskCreate(task_print_5s, "print5s",  2*1024, NULL, 4, NULL);
    xTaskCreate(task_console,  "console",  4*1024, NULL, 5, NULL);
}
