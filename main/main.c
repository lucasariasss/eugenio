// main.c

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#include "wifi_app.h"
#include "msg_app.h"
#include "esp_log.h"

#define MASTER 1
#define THERMAL 0
#define PIR_UNIT 0

#if (THERMAL + PIR_UNIT + MASTER) > 1
#error "Only one application role can be enabled at a time"
#elif (THERMAL + PIR_UNIT + MASTER) == 0
#error "At least one application role must be enabled"
#endif

#if MASTER == 1
#include "lwip/netdb.h"
#include "console_app.h"

#define TAG "MASTER"
#endif // MASTER

#if THERMAL == 1
#include "freertos/semphr.h"

#include "lm35_app.h"
#include "cooler_app.h"

#define TAG "THERMAL"

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
#endif // THERMAL

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGE(TAG, "App iniciada");

#if MASTER == 1
    wifi_app_init_sta();
    wifi_app_wait_sta_ip();  
    msg_app_open_master();

    xTaskCreate(msg_app_task_tx_hello, "udp_tx_hello", 2*1024, NULL, 5, NULL);
    xTaskCreate(msg_app_task_rx_master,   "udp_rx",   3*1024, NULL, 6, NULL);
    xTaskCreate(console_app_task_print_5s, "print5s",  2*1024, NULL, 4, NULL);
    xTaskCreate(console_app_task,  "console",  4*1024, NULL, 5, NULL);
#endif // MASTER
    
#if THERMAL == 1
    wifi_app_init_softap();
    msg_app_open_slave();
    lm35_app_init();
    cooler_app_pwm_init();
    sock_mutex = xSemaphoreCreateMutex();

    xTaskCreate(msg_app_task_rx_slave, "udp_rx", 3*1024, NULL, 6, NULL);
    xTaskCreate(task_sense_ctrl_tx, "sense_tx", 3*1024, NULL, 5, NULL);
#endif // THERMAL
}
