#ifndef LM35_APP_H
#define LM35_APP_H
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// ADC LM35
#define LM35_ADC_CH   ADC_CHANNEL_6     // GPIO34
#define LM35_BITW     ADC_BITWIDTH_12
#define LM35_ATTEN    ADC_ATTEN_DB_12

void lm35_app_init(void);

float lm35_app_celsius(void);

#endif // LM35_APP_H