// main.c

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "wifi_app.h"
#include "msg_app.h"
#include "esp_log.h"

#include "app_role.h"
#include "aux_app.h"
#include "lm35_app.h"
#include "cooler_app.h"
#include "console_app.h"

#if MASTER == 1
#define TAG "MASTER"
#endif // MASTER

#if THERMAL == 1
#define TAG "THERMAL"
#endif // THERMAL


#if AUX == 1
#define TAG "AUX"
#endif  // AUX


void app_main(void){
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_LOGE(TAG, "App iniciada");

#if MASTER == 1
    wifi_app_init_softap();
    msg_app_open_master();

    xTaskCreate(console_app_task,  "console",  4*1024, NULL, 5, NULL);
#endif // MASTER

#if SLAVE == 1
    wifi_app_connect_sta();
    msg_app_open_slave();
#endif // AUX
    
#if THERMAL == 1
    lm35_app_init();
    cooler_app_pwm_init();
    xTaskCreate(cooler_app_task_sense_ctrl_tx, "sense_tx", 3*1024, NULL, 5, NULL);
#endif // THERMAL

#if AUX == 1
    aux_app_init();
    xTaskCreate(aux_app_poll_task, "aux_app_poll_task", 2048, NULL, 4, NULL);
    xTaskCreate(aux_app_led_task, "aux_app_led_task", 2048, NULL, 4, NULL);
#endif

    xTaskCreate(msg_app_task_rx, "udp_rx", 3*1024, NULL, 6, NULL);
}