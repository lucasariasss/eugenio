#include "cooler_app.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include <math.h>

#define TAG "cooler_app: "

void cooler_app_pwm_init(void) {
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

void cooler_app_set_pct(float pct){
    if(pct<0) pct=0;
    if(pct>100) pct=100;
    int duty = lroundf(pct * 1023.0f / 100.0f);
    ledc_set_duty(FAN_SPD, FAN_CH, duty);
    ledc_update_duty(FAN_SPD, FAN_CH);
    
    if (getchar() == 't')
    {
        ESP_LOGI(TAG, "FAN set to %.1f%% (duty=%d)", pct, duty);
    }
}

float cooler_app_curve(float t, float sp){
    float t0=sp, t100=sp+15.0f;
    if (t<=t0) return 0;
    if (t>=t100) return 100;
    return (t - t0) * 100.0f / (t100 - t0);
}