#include "aux_app.h"
#include "msg_app.h"      // para g_pir / g_sw
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#define AUX_LED_GPIO  GPIO_NUM_25
#define AUX_SW_GPIO   GPIO_NUM_26
#define AUX_PIR_GPIO  GPIO_NUM_34

static const char *TAG = "aux_app";

static TaskHandle_t s_aux_task_handle = NULL;

/* Inicializar la aplicación auxiliar.
 * Devuelve ESP_OK si se creó la tarea correctamente. */
esp_err_t aux_app_init(void)
{
    ESP_LOGI(TAG, "Inicializando aplicación auxiliar");
    ESP_LOGI(TAG, "Configurando LED en GPIO %d", AUX_LED_GPIO);
    gpio_config_t led = {
        .pin_bit_mask = 1ULL << AUX_LED_GPIO,
        .mode = GPIO_MODE_OUTPUT, .pull_up_en = 0, .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&led));
    ESP_ERROR_CHECK(gpio_set_level(AUX_LED_GPIO, 0));

    ESP_LOGI(TAG, "Configurando SW en GPIO %d", AUX_SW_GPIO);
    gpio_config_t sw = {
        .pin_bit_mask = 1ULL << AUX_SW_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1, .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&sw));

    ESP_LOGI(TAG, "Configurando PIR en GPIO %d", AUX_PIR_GPIO);
    gpio_config_t pir = {
        .pin_bit_mask = 1ULL << AUX_PIR_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&pir));

    return ESP_OK;
}

int aux_app_read_pir(void)
{
    return gpio_get_level(AUX_PIR_GPIO);
}

esp_err_t aux_app_set_led(int on)
{
    return gpio_set_level(AUX_LED_GPIO, on ? 1 : 0);
}

void aux_app_poll_task(void *arg){
    for(;;){
        int pir = aux_app_read_pir();
        char line[32];
        if (g_pir != pir)
        {
            g_pir = pir;
            ESP_LOGI(TAG, "Estado de PIR: %d", g_pir);
            int len = snprintf(line, sizeof(line), "PIR:%d\n", pir);
            if (len > 0) msg_app_tx_to_master(line);
        }
        
        int sw  = !gpio_get_level(AUX_SW_GPIO);
        if (g_sw != sw)
        {
            g_sw = sw;
            ESP_LOGI(TAG, "Estado de switch: %d", g_sw);
            int len = snprintf(line, sizeof(line), "SW:%d\n", sw);
            if (len > 0) msg_app_tx_to_master(line);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void aux_app_led_task(void *arg){
    for(;;){
        int led_on = 0;
        switch (g_led_src) {
            case LED_SRC_PIR:     led_on = g_pir;          break;
            case LED_SRC_SWITCH:  led_on = g_sw;           break;
            case LED_SRC_CONSOLE: led_on = g_cmd_led;      break;
        }
        aux_app_set_led(led_on);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}