#include "msg_app.h"

#include <string.h>
#include <stdlib.h> 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"

int udp_sock = -1;
struct sockaddr_in master_addr = {0};
bool master_known = false;
SemaphoreHandle_t sock_mutex = NULL;
float setpoint_c = 30.0f;

#define TAG "msg_app: "

void msg_app_open_slave(void) {
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_sock < 0) { ESP_LOGE(TAG, "socket failed"); vTaskDelay(portMAX_DELAY); }
    struct sockaddr_in listen_addr = {0};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(UDP_PORT);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(udp_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        ESP_LOGE(TAG, "bind failed");
        vTaskDelay(portMAX_DELAY);
    }
    ESP_LOGI(TAG, "UDP bind en *:%d", UDP_PORT);
}

// RX de comandos UDP: "HELLO" o "SET:<float>"
void msg_app_task_rx_slave(void *arg){
    char buf[64];
    struct sockaddr_in src; socklen_t slen=sizeof(src);
    while (1) {
        int n = recvfrom(udp_sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&src, &slen);
        if (n>0){
            buf[n]=0;
            // Recordar master para telemetría
            xSemaphoreTake(sock_mutex, portMAX_DELAY);
            master_addr = src; master_known = true;
            xSemaphoreGive(sock_mutex);

            if (strncmp(buf,"SET:",4)==0){
                float v = atof(buf+4);
                if (v>0 && v<120){ setpoint_c = v; ESP_LOGI(TAG,"Nuevo setpoint: %.2f C", setpoint_c); }
            }
        }
    }
}