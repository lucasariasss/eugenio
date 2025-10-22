// msg_app.c

#include "msg_app.h"

#include <string.h>
#include <stdlib.h> 
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"

#include "app_role.h"

int udp_sock = -1;
struct sockaddr_in master_addr = {0};
struct sockaddr_in slave_addr = {0};

volatile bool master_known = false;
volatile bool slave_known = false;
SemaphoreHandle_t sock_mutex = NULL;
float setpoint_c = 30.0f;
volatile float last_temp = NAN;
static TickType_t last_temp_tick = 0;

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

    master_addr.sin_family = AF_INET;
    master_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, MASTER_IP, &master_addr.sin_addr);
    master_known = true;

    const char *hello = "HELLO\n";
    sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&master_addr, sizeof(master_addr));
    ESP_LOGI(TAG, "HELLO enviado a maestro %s:%d", MASTER_IP, UDP_PORT);
}

void msg_app_open_master(void){
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_sock < 0){ ESP_LOGE(TAG,"socket failed"); vTaskDelay(portMAX_DELAY); }
    last_temp_tick = 0;

    struct timeval tv = { .tv_sec = 0, .tv_usec = 200 * 1000 };
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in listen_addr = {0};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(UDP_PORT);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(udp_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        ESP_LOGE(TAG, "bind failed");
        vTaskDelay(portMAX_DELAY);
    }
    memset(&slave_addr, 0, sizeof(slave_addr));
    slave_known = false;

    ESP_LOGI(TAG, "UDP maestro escuchando en *:%d", UDP_PORT);
}

void msg_app_task_rx(void *arg){
    char buf[64];
    struct sockaddr_in src; socklen_t slen=sizeof(src);
#if MASTER == 1
    const TickType_t hello_timeout = pdMS_TO_TICKS(1000); // si pasa 1s sin TEMP -> HELLO
    TickType_t now;
#endif
    while (1){
        int n = recvfrom(udp_sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&src, &slen);
        if (n>0){
            buf[n]=0;
#if THERMAL == 1
            master_addr = src; master_known = true;

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
                ESP_LOGI(TAG, "HELLO recibido del maestro %s", ip);
            }
#endif // THERMAL
#if MASTER == 1
            slave_addr = src; slave_known = true;

            if (strncmp(buf, "HELLO", 5) == 0) {
                char ip[16]; inet_ntop(AF_INET, &src.sin_addr, ip, sizeof(ip));
                ESP_LOGI(TAG, "HELLO recibido desde esclavo %s", ip);
                last_temp_tick = xTaskGetTickCount(); // mantiene vivo el watchdog HELLO
                continue;
            }
            if (strncmp(buf,"TEMP:",5)==0){
                last_temp = atof(buf+5);
                last_temp_tick = xTaskGetTickCount();
            }
        } else {
            now = xTaskGetTickCount();
            bool stale = (last_temp_tick == 0) || ((now - last_temp_tick) > hello_timeout);
            if (stale && slave_known){
                static const char *hello = "HELLO\n";
                sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
            }
#endif // MASTER
        }
    }
}