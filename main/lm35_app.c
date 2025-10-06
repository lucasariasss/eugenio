#include "lm35_app.h"
#include "esp_log.h"
#include <stdio.h>

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t         adc_cali_handle;
static bool                      adc_cali_enabled = false;

#define TAG "lm35_app: "

void lm35_app_init(void)
{
    ESP_LOGI(TAG, "Inicializando ADC LM35...");
    // Unidad ADC1 en modo one-shot
    adc_oneshot_unit_init_cfg_t unit_cfg = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    // Canal (GPIO34) + atenuación 12 dB
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = LM35_BITW,
        .atten    = LM35_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LM35_ADC_CH, &chan_cfg));

    // Calibración (usa eFuse si está disponible)
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = LM35_ATTEN,
        .bitwidth = LM35_BITW,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_cali_handle) == ESP_OK) {
        adc_cali_enabled = true;
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = LM35_ATTEN,
        .bitwidth = LM35_BITW,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &adc_cali_handle) == ESP_OK) {
        adc_cali_enabled = true;
    }
#endif
}

float lm35_app_celsius(void)
{
    const int samples = 16;
    int acc = 0, raw = 0;
    for (int i = 0; i < samples; ++i) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, LM35_ADC_CH, &raw));
        acc += raw;
    }
    int avg = acc / samples;

    int mv = 0;
    if (adc_cali_enabled) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, avg, &mv)); // mV reales
    } else {
        // Aprox si no hay eFuse de calibración: 3.3V a 12 bits
        mv = (avg * 3300) / 4095;
    }

    if ((getchar()) == 't')
    {
        ESP_LOGI(TAG, "ADC raw %d -> %d mV -> %f c", avg, mv, (float)mv / 10.0f);
    }
    

    // LM35: 10 mV/°C
    return (float)mv / 10.0f;
}