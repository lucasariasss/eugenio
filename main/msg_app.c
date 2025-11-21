// msg_app.c

#include "msg_app.h"

#include <string.h>
#include <stdlib.h> 
#include <math.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "lwip/inet.h"
#include "lwip/sockets.h"

#include "app_role.h"

int udp_sock = -1;
struct sockaddr_in master_addr = {0};
struct sockaddr_in slave_addr_thermal = {0};
struct sockaddr_in slave_addr_aux     = {0};

volatile bool master_known = false;
volatile bool thermal_known = false;
volatile bool aux_known     = false;
volatile float last_temp = NAN;
static TickType_t last_temp_tick = 0;

volatile cool_src_t g_cool_src = COOL_SRC_TEMP;
volatile led_src_t  g_led_src  = LED_SRC_PIR;
volatile float      g_setpoint = 30.0f;
volatile int        g_cmd_led  = 0;
volatile int        g_sw       = 0;
volatile int        g_pir      = 0;

#define TAG "msg_app: "

int msg_app_tx_to_thermal(const char *s){
    if (!thermal_known) return -1;
    return sendto(udp_sock, s, strlen(s), 0, (struct sockaddr*)&slave_addr_thermal, sizeof(slave_addr_thermal));
}

int msg_app_tx_to_aux(const char *s){
    if (!aux_known) return -1;
    return sendto(udp_sock, s, strlen(s), 0, (struct sockaddr*)&slave_addr_aux, sizeof(slave_addr_aux));
}

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

    #if THERMAL == 1
        const char *ping = "PING:THERMAL\n";
    #elif AUX == 1
        const char *ping = "PING:AUX\n";
    #else
        const char *ping = "PING\n";
    #endif
    sendto(udp_sock, ping, strlen(ping), 0, (struct sockaddr*)&master_addr, sizeof(master_addr));
    ESP_LOGI(TAG, "SLAVE enlazado UDP en *:%d, PING a %s:%d", UDP_PORT, MASTER_IP, UDP_PORT);
}

void msg_app_open_master(void){
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_sock < 0){ ESP_LOGE(TAG,"socket failed"); vTaskDelay(portMAX_DELAY); }
    last_temp_tick = 0;

    struct timeval tv = { .tv_sec = 0, .tv_usec = 200 * 1000 };
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in listen_addr = {0};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port   = htons(UDP_PORT);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(udp_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) < 0) {
        ESP_LOGE(TAG, "bind failed");
        vTaskDelay(portMAX_DELAY);
    }

    memset(&slave_addr_thermal, 0, sizeof(slave_addr_thermal));
    memset(&slave_addr_aux,     0, sizeof(slave_addr_aux));
    thermal_known = false;
    aux_known     = false;

    ESP_LOGI(TAG, "MASTER escuchando UDP en *:%d", UDP_PORT);
}


/**************** MAQUINAS DE ESTADO POR ROL ****************/
#if MASTER == 1
static void msg_app_handle_master(const char *buf, const struct sockaddr_in *src)
{
    if (!strncmp(buf,"TEMP:",5)){
        last_temp = atof(buf+5);
        slave_addr_thermal = *src; thermal_known = true;
        last_temp_tick = xTaskGetTickCount();
        return;
    }
    if (!strncmp(buf,"PING:THERMAL",12)){
        slave_addr_thermal = *src; thermal_known = true;
        last_temp_tick = xTaskGetTickCount();
        return;
    }
    if (!strncmp(buf,"PING:AUX",8)){
        slave_addr_aux = *src; aux_known = true;
        return;
    }
    if (!strncmp(buf,"PING",4)){
        last_temp_tick = xTaskGetTickCount();
        return;
    }
}

#endif // MASTER

#if THERMAL == 1
static void msg_app_handle_thermal(const char *buf, const struct sockaddr_in *src)
{
    if (!strncmp(buf,"CFG:COOLER_SRC=TEMP",19))   g_cool_src = COOL_SRC_TEMP;
    else if (!strncmp(buf,"CFG:COOLER_SRC=PIR",18)) g_cool_src = COOL_SRC_PIR;
    else if (!strncmp(buf,"CFG:COOLER_SRC=SWITCH",21)) g_cool_src = COOL_SRC_SWITCH;
    else if (!strncmp(buf,"CFG:COOLER_SRC=OFF",18)) g_cool_src = COOL_SRC_OFF;

    if (!strncmp(buf,"SET:",4)){
        float v = atof(buf+4);
        if (v>0 && v<120)
        {
            g_setpoint = v; 
            ESP_LOGI(TAG,"Nuevo setpoint: %.2f C", g_setpoint); 
        }
    }

    if (!strncmp(buf,"SW:",3)){
        g_sw = atoi(buf+3);
        if(g_sw == 0 || g_sw == 1)
        {
            ESP_LOGI(TAG,"Estado de switch: %d", g_sw);
        }
        else
        {
            g_sw = 0;
            ESP_LOGE(TAG,"ingresa un valor valido del switch");
        }
    }

    if (!strncmp(buf,"PIR:",4)){
        int pir = atoi(buf+4);
        if(pir == 0 || pir == 1)
        {
            ESP_LOGI(TAG,"Estado de PIR: %d", pir);
        }
        else
        {
            pir = 0;
            ESP_LOGE(TAG,"ingresa un valor valido del pir");
        }
    }
}
#endif // THERMAL

#if AUX == 1
static void msg_app_handle_aux(const char *buf, const struct sockaddr_in *src)
{
    if (!strncmp(buf,"CFG:LED_SRC=PIR",15))          g_led_src = LED_SRC_PIR;
    else if (!strncmp(buf,"CFG:LED_SRC=SWITCH",18))    g_led_src = LED_SRC_SWITCH;
    else if (!strncmp(buf,"CFG:LED_SRC=CONSOLE",19))   g_led_src = LED_SRC_CONSOLE;

    if (!strncmp(buf,"CMD:LED=",8))
    {
        int v = atoi(buf+8);
        g_cmd_led = (v!=0);
    }
}
#endif // AUX

void msg_app_task_rx(void *arg){
    char buf[64];
    struct sockaddr_in src;
    socklen_t slen=sizeof(src);

#if MASTER == 1
    const TickType_t ping_timeout = pdMS_TO_TICKS(1000); // si pasa 1s sin TEMP -> PING
    TickType_t now;
#endif // MASTER

    while (1){
        int n = recvfrom(udp_sock, buf, sizeof(buf)-1, 0, (struct sockaddr*)&src, &slen);
        if (n>0){
            buf[n]=0;
            for(char *p=buf; *p; ++p) *p = toupper((unsigned char)*p);

#if MASTER == 1
            msg_app_handle_master(buf, &src);
#endif // MASTER

#if THERMAL == 1
            msg_app_handle_thermal(buf, &src);
#endif // THERMAL

#if AUX == 1
            msg_app_handle_aux(buf, &src);
#endif // AUX == 1
        }
#if MASTER == 1
        else {
            now = xTaskGetTickCount();
            bool stale = (last_temp_tick == 0) || ((now - last_temp_tick) > ping_timeout);
            if (stale){
                if (thermal_known) sendto(udp_sock,"PING:MASTER\n",12,0, (struct sockaddr*)&slave_addr_thermal,sizeof(slave_addr_thermal));
                if (aux_known) sendto(udp_sock,"PING:MASTER\n",12,0, (struct sockaddr*)&slave_addr_aux,sizeof(slave_addr_aux));
            }
        }
#endif // MASTER
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}