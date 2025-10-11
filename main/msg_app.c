// msg_app.c

#include "msg_app.h"

#include <string.h>
#include <stdlib.h> 
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

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

    memset(&slave_addr, 0, sizeof(slave_addr));
    slave_addr.sin_family = AF_INET;
    slave_addr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, SLAVE_IP, &slave_addr.sin_addr);

    // enviar HELLO para que el esclavo recuerde nuestra IP/puerto
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
                    msg_app_setpoint_save_nvs(v);
                    ESP_LOGI(TAG,"Nuevo setpoint: %.2f C", setpoint_c); 
                }
            }
            if (strncmp(buf, "HELLO", 5) == 0) {
                char ip[16]; inet_ntop(AF_INET, &src.sin_addr, ip, sizeof(ip));
                ESP_LOGI(TAG, "HELLO desde %s:%u", ip, ntohs(src.sin_port));
            }
        }
    }
}

// RX de "TEMP:x.y"
void msg_app_task_rx_master(void *arg){
    char buf[64];
    struct sockaddr_in src; socklen_t slen=sizeof(src);
    while (1){
        int n = recvfrom(udp_sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&src, &slen);
        if (n>0){
            buf[n]=0;
            if (strncmp(buf,"TEMP:",5)==0){
                last_temp = atof(buf+5);
                last_temp_tick = xTaskGetTickCount();
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

void msg_app_task_tx_hello(void *arg){
    // Reintenta hasta que last_temp deje de ser NaN (ya llegó algún TEMP)
    while (isnan(last_temp)) {
        const char *hello = "HELLO\n";
        sendto(udp_sock, hello, strlen(hello), 0,
               (struct sockaddr*)&slave_addr, sizeof(slave_addr));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_LOGI(TAG, "HELLO confirmado (ya llegan TEMP), detengo tx_hello");
    vTaskDelete(NULL);
}

void msg_app_task_link_supervisor(void *arg){
    const TickType_t timeout = pdMS_TO_TICKS(3000);  // 3 s sin TEMP => relanzar HELLO
    for(;;){
        TickType_t now = xTaskGetTickCount();
        bool stale = (last_temp_tick == 0) || ((now - last_temp_tick) > timeout);
        if (stale){
            const char *hello = "HELLO\n";
            sendto(udp_sock, hello, strlen(hello), 0,
                   (struct sockaddr*)&slave_addr, sizeof(slave_addr));
        }
        vTaskDelay(pdMS_TO_TICKS(500));  // reintento cada 500 ms
    }
}

esp_err_t msg_app_setpoint_save_nvs(float v){
    nvs_handle_t h; esp_err_t err = nvs_open("app", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, "setpoint_c", &v, sizeof(v));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

void msg_app_setpoint_load_nvs(float *out){
    nvs_handle_t h; size_t sz = sizeof(*out);
    if (nvs_open("app", NVS_READONLY, &h) == ESP_OK){
        if (nvs_get_blob(h, "setpoint_c", out, &sz) == ESP_OK && sz == sizeof(*out)){
            // ok
        }
        nvs_close(h);
    }
}