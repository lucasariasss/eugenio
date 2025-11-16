// cooler_app.c

#include "cooler_app.h"
#include "lm35_app.h"
#include "msg_app.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include <math.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "cooler_app: "

/* ====== Parámetros configurables ====== */
static const float DUTY_MIN_ON   = 25.0f;   // % mínimo mientras el fan está ON
static const float DUTY_MAX      = 100.0f;  // % máximo
static const float DELTA_ON_C    = 0.7f;    // encender cuando T >= SP + 0.7°C
static const float DELTA_OFF_C   = 0.4f;    // apagar cuando T <= SP - 0.4°C

/* ====== Estado interno ====== */
static bool  fan_on   = false;

/* ====== Utilidades ====== */
static inline float clampf(float x, float lo, float hi){
    return (x < lo) ? lo : (x > hi ? hi : x);
}

/* Map lineal:
   - Enciende a partir de (SP + DELTA_ON_C) con DUTY_MIN_ON
   - Llega a 100% en (SP + 5°C)  (ajustable)
*/
static float cooler_app_funcion_lineal(float temp, float sp){
    float x0 = sp + DELTA_ON_C;
    float x1 = sp + 5.0f; // a +5°C del setpoint queremos full

    if (temp <= x0) return 0.0f;
    if (temp >= x1) return DUTY_MAX;

    float l0a1 = (temp - x0) / (x1 - x0);
    float l25a100 = DUTY_MIN_ON + l0a1 * (DUTY_MAX - DUTY_MIN_ON);

    return clampf(l25a100, DUTY_MIN_ON, DUTY_MAX);
}

/* ====== Init PWM ====== */
void cooler_app_pwm_init(void) {
    ledc_timer_config_t tcfg = {
        .speed_mode = FAN_SPD,
        .timer_num = FAN_TM,
        .duty_resolution = FAN_RES,
        .freq_hz = FAN_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&tcfg);

    ledc_channel_config_t ccfg = {
        .gpio_num = FAN_PWM_PIN,
        .speed_mode = FAN_SPD,
        .channel = FAN_CH,
        .timer_sel = FAN_TM,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ccfg);
}

/* ====== Aplicación del PWM con duty mínimo ====== */
void cooler_app_set_duty_percent(float duty_percent){
    duty_percent = clampf(duty_percent, 0.0f, 100.0f);

    // Si está encendido y el % es bajo, respetar duty mínimo
    if (duty_percent > 0.0f && duty_percent < DUTY_MIN_ON) duty_percent = DUTY_MIN_ON;

    int duty = lroundf(duty_percent * 1023.0f / 100.0f); // 10-bit: 0..1023
    ledc_set_duty(FAN_SPD, FAN_CH, duty);
    ledc_update_duty(FAN_SPD, FAN_CH);

    
    if ((getchar()) == 't')
    {
        ESP_LOGI(TAG, "FAN = %.1f%% (duty=%d)", duty_percent, duty);
    }
}

/* ====== Curva con histeresis ======
   - fan_on pasa a true cuando tc >= sp + DELTA_ON_C
   - fan_on pasa a false cuando tc <= sp - DELTA_OFF_C
   - mientras esté ON, usa curva lineal con mínimo DUTY_MIN_ON
*/
float cooler_app_curve(float tc, float sp){
    // Histeresis ON/OFF
    if (!fan_on && tc >= sp + DELTA_ON_C)  fan_on = true;
    if ( fan_on && tc <= sp - DELTA_OFF_C) fan_on = false;

    if (!fan_on) return 0.0f;

    return cooler_app_funcion_lineal(tc, sp);
}

// Sensado + control + TX periódica "TEMP:x.y"
void cooler_app_task_sense_ctrl_tx(void *arg){
    TickType_t last_sense = xTaskGetTickCount();
    TickType_t last_tx = xTaskGetTickCount();
    char line[32];

    while (1){
        float temp_c = lm35_app_celsius(), duty_percent = 0.0f;
        int pir = g_pir, sw  = g_sw;
        float sp = g_setpoint;         
        switch (g_cool_src) {
            case COOL_SRC_TEMP:   duty_percent = cooler_app_curve(temp_c, sp); break;
            case COOL_SRC_PIR:    duty_percent = pir ? 100.0f : 0.0f;          break;
            case COOL_SRC_SWITCH: duty_percent = sw  ? 100.0f : 0.0f;          break;
            case COOL_SRC_OFF:    duty_percent = 0.0f;                         break;
        }
        cooler_app_set_duty_percent(duty_percent);

        if (xTaskGetTickCount() - last_tx >= pdMS_TO_TICKS(1000)){
            int len = snprintf(line, sizeof(line), "TEMP:%.2f\n", temp_c);
            bool ok = master_known;
            struct sockaddr_in dst = master_addr;
            if (ok) sendto(udp_sock, line, len, 0, (struct sockaddr*)&dst, sizeof(dst));
            last_tx = xTaskGetTickCount();
        }
        vTaskDelayUntil(&last_sense, pdMS_TO_TICKS(100)); // 10 Hz
    }
}