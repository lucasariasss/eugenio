// main.c

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "wifi_app.h"
#include "msg_app.h"
#include "esp_log.h"

#include "app_role.h"

#if MASTER == 1
#include "console_app.h"

#define TAG "MASTER"
#endif // MASTER

#if THERMAL == 1
#include "lm35_app.h"
#include "cooler_app.h"

#define TAG "THERMAL"
#endif // THERMAL

void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGE(TAG, "App iniciada");

#if MASTER == 1
    wifi_app_connect_sta();
    msg_app_open_master();

    xTaskCreate(console_app_task,  "console",  4*1024, NULL, 5, NULL);
#endif // MASTER
    
#if THERMAL == 1
    ESP_LOGI("THERMAL", "Setpoint restaurado: %.2f C", setpoint_c);
    wifi_app_init_softap();
    msg_app_open_slave();
    lm35_app_init();
    cooler_app_pwm_init();

    xTaskCreate(cooler_app_task_sense_ctrl_tx, "sense_tx", 3*1024, NULL, 5, NULL);
#endif // THERMAL

    xTaskCreate(msg_app_task_rx, "udp_rx", 3*1024, NULL, 6, NULL);
}