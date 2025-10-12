// msg_app.c

#include "msg_app.h"

#include <string.h>
#include <stdlib.h> 
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"

int udp_sock = -1;
struct sockaddr_in master_addr = {0};
struct sockaddr_in slave_addr;

bool master_known = false;
SemaphoreHandle_t sock_mutex = NULL;
float setpoint_c = 30.0f;
volatile float last_temp = NAN;
volatile TickType_t last_temp_tick = 0;

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

void msg_app_open_master(void){
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_sock < 0){ ESP_LOGE(TAG,"socket failed"); vTaskDelay(portMAX_DELAY); }
    last_temp_tick = 0;

    struct timeval tv = { .tv_sec = 0, .tv_usec = 200 * 1000 };
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    memset(&slave_addr, 0, sizeof(slave_addr));
    slave_addr.sin_family = AF_INET;
    slave_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, SLAVE_IP, &slave_addr.sin_addr);

    const char *hello = "HELLO\n";
    sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
    ESP_LOGI(TAG, "UDP listo hacia %s:%d", SLAVE_IP, UDP_PORT);
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
                if (v>0 && v<120)
                {
                    setpoint_c = v; 
                    ESP_LOGI(TAG,"Nuevo setpoint: %.2f C", setpoint_c); 
                }
            }
            if (strncmp(buf, "HELLO", 5) == 0) {
                char ip[16]; inet_ntop(AF_INET, &src.sin_addr, ip, sizeof(ip));
            }
        }
    }
}

// RX de "TEMP:x.y"
void msg_app_task_rx_master(void *arg){
    char buf[64];
    struct sockaddr_in src; socklen_t slen=sizeof(src);

    const TickType_t hello_timeout = pdMS_TO_TICKS(1000); // si pasa 1s sin TEMP -> HELLO
    TickType_t now;

    while (1){
        int n = recvfrom(udp_sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&src, &slen);
        if (n>0){
            buf[n]=0;
            if (strncmp(buf,"TEMP:",5)==0){
                last_temp = atof(buf+5);
                last_temp_tick = xTaskGetTickCount();
            }
        } else {
            now = xTaskGetTickCount();
            bool stale = (last_temp_tick == 0) || ((now - last_temp_tick) > hello_timeout);
            if (stale){
                static const char *hello = "HELLO\n";
                sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
            }
        }
    }
}