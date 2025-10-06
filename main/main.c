// main.c (ESCLAVO) - ESP-IDF
#include <stdio.h>
#include <math.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "driver/ledc.h"

#include "wifi_app.h"
#include "lm35_app.h"

#define TAG "SLAVE"

// UDP
#define UDP_PORT     3333

// PWM FAN
#define FAN_PWM_PIN  GPIO_NUM_14
#define FAN_FREQ_HZ  25000
#define FAN_RES      LEDC_TIMER_10_BIT
#define FAN_CH       LEDC_CHANNEL_0
#define FAN_TM       LEDC_TIMER_0
#define FAN_SPD      LEDC_HIGH_SPEED_MODE

static float setpoint_c = 30.0f;

static int udp_sock = -1;
static struct sockaddr_in master_addr;
static bool master_known = false;
static SemaphoreHandle_t sock_mutex;

static void udp_open(void) {
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

static void cooler_app_pwm_init(void) {
    ledc_timer_config_t tcfg = {
        .speed_mode = FAN_SPD, .timer_num = FAN_TM,
        .duty_resolution = FAN_RES, .freq_hz = FAN_FREQ_HZ, .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&tcfg);
    ledc_channel_config_t ccfg = {
        .gpio_num = FAN_PWM_PIN, .speed_mode = FAN_SPD, .channel = FAN_CH,
        .timer_sel = FAN_TM, .duty = 0, .hpoint = 0
    };
    ledc_channel_config(&ccfg);
}

static void cooler_app_set_pct(float pct){
    if(pct<0) pct=0;
    if(pct>100) pct=100;
    uint32_t duty = lroundf(pct * 1023.0f / 100.0f);
    ledc_set_duty(FAN_SPD, FAN_CH, duty);
    ledc_update_duty(FAN_SPD, FAN_CH);
}

static float cooler_app_curve(float t, float sp){
    float t0=sp, t100=sp+15.0f;
    if (t<=t0) return 0;
    if (t>=t100) return 100;
    return (t - t0) * 100.0f / (t100 - t0);
}

// RX de comandos UDP: "HELLO" o "SET:<float>"
static void task_udp_rx(void *arg){
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

// Sensado + control + TX periódica "TEMP:x.y"
static void task_sense_ctrl_tx(void *arg){
    TickType_t last_sense = xTaskGetTickCount();
    TickType_t last_tx = xTaskGetTickCount();
    char line[32];

    while (1){
        float tc = lm35_app_celsius();
        float pct = cooler_app_curve(tc, setpoint_c);
        cooler_app_set_pct(pct);

        if (xTaskGetTickCount() - last_tx >= pdMS_TO_TICKS(1000)){
            int len = snprintf(line, sizeof(line), "TEMP:%.2f\n", tc);
            xSemaphoreTake(sock_mutex, portMAX_DELAY);
            bool ok = master_known;
            struct sockaddr_in dst = master_addr;
            xSemaphoreGive(sock_mutex);
            if (ok) sendto(udp_sock, line, len, 0, (struct sockaddr*)&dst, sizeof(dst));
            last_tx = xTaskGetTickCount();
        }
        vTaskDelayUntil(&last_sense, pdMS_TO_TICKS(100)); // 10 Hz
    }
}

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_app_init_softap();
    udp_open();
    lm35_app_init();
    cooler_app_pwm_init();
    sock_mutex = xSemaphoreCreateMutex();

    xTaskCreate(task_udp_rx, "udp_rx", 3*1024, NULL, 6, NULL);
    xTaskCreate(task_sense_ctrl_tx, "sense_tx", 3*1024, NULL, 5, NULL);
}
