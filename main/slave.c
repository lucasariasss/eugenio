// main.c (ESCLAVO) - ESP-IDF
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "driver/ledc.h"

#define TAG "SLAVE"

// WiFi SoftAP
#define AP_SSID      "LM35_SLAVE_AP"
#define AP_PASS      "12345678"
#define AP_CHANNEL   1
#define AP_MAX_CONN  2

// UDP
#define UDP_PORT     3333

// ADC LM35
#define LM35_ADC_CH  ADC1_CHANNEL_6   // GPIO34
#define LM35_ATTEN   ADC_ATTEN_DB_11
#define LM35_WIDTH   ADC_WIDTH_BIT_12

// PWM FAN
#define FAN_PWM_PIN  GPIO_NUM_14
#define FAN_FREQ_HZ  25000
#define FAN_RES      LEDC_TIMER_10_BIT
#define FAN_CH       LEDC_CHANNEL_0
#define FAN_TM       LEDC_TIMER_0
#define FAN_SPD      LEDC_HIGH_SPEED_MODE

static esp_adc_cal_characteristics_t adc_chars;
static float setpoint_c = 30.0f;

static int udp_sock = -1;
static struct sockaddr_in master_addr;
static bool master_known = false;
static SemaphoreHandle_t sock_mutex;

static void wifi_init_softap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = { 0 };
    strcpy((char*)wifi_config.ap.ssid, AP_SSID);
    wifi_config.ap.ssid_len = strlen(AP_SSID);
    strcpy((char*)wifi_config.ap.password, AP_PASS);
    wifi_config.ap.channel = AP_CHANNEL;
    wifi_config.ap.max_connection = AP_MAX_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    if (strlen(AP_PASS) == 0) wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP iniciado: SSID=%s, pass=%s, canal=%d",
             AP_SSID, AP_PASS, AP_CHANNEL);
}

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

static void adc_init(void) {
    adc1_config_width(LM35_WIDTH);
    adc1_config_channel_atten(LM35_ADC_CH, LM35_ATTEN);
    esp_adc_cal_characterize(ADC_UNIT_1, LM35_ATTEN, LM35_WIDTH, 1100, &adc_chars);
}

static float lm35_celsius(void) {
    const int samples = 16;
    uint32_t acc = 0;
    for (int i=0;i<samples;i++) acc += adc1_get_raw(LM35_ADC_CH);
    uint32_t raw = acc / samples;
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
    return (float)mv / 10.0f; // 10 mV/°C
}

static void pwm_init(void) {
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

static void fan_set_pct(float pct){
    if (pct<0) pct=0; if (pct>100) pct=100;
    uint32_t duty = lroundf(pct * 1023.0f / 100.0f);
    ledc_set_duty(FAN_SPD, FAN_CH, duty);
    ledc_update_duty(FAN_SPD, FAN_CH);
}

static float fan_curve(float t, float sp){
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
        float tc = lm35_celsius();
        float pct = fan_curve(tc, setpoint_c);
        fan_set_pct(pct);

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
    wifi_init_softap();
    udp_open();
    adc_init();
    pwm_init();
    sock_mutex = xSemaphoreCreateMutex();

    xTaskCreate(task_udp_rx, "udp_rx", 3*1024, NULL, 6, NULL);
    xTaskCreate(task_sense_ctrl_tx, "sense_tx", 3*1024, NULL, 5, NULL);
}
