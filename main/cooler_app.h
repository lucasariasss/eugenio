#ifndef COOLER_APP_H
#define COOLER_APP_H

// PWM FAN
#define FAN_PWM_PIN  GPIO_NUM_14
#define FAN_FREQ_HZ  25000
#define FAN_RES      LEDC_TIMER_10_BIT
#define FAN_CH       LEDC_CHANNEL_0
#define FAN_TM       LEDC_TIMER_0
#define FAN_SPD      LEDC_HIGH_SPEED_MODE

void cooler_app_pwm_init(void);

void cooler_app_set_pct(float pct);

float cooler_app_curve(float t, float sp);

#endif // COOLER_APP_H