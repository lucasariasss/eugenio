#include "aux_app.h"
#include "msg_app.h"      // para g_pir / g_sw
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "aux_app";

/* Configuración por defecto (ajustar según necesidades) */
#ifndef AUX_APP_TASK_STACK_SIZE
#define AUX_APP_TASK_STACK_SIZE 4096
#endif

#ifndef AUX_APP_TASK_PRIO
#define AUX_APP_TASK_PRIO      (tskIDLE_PRIORITY + 5)
#endif

static TaskHandle_t s_aux_task_handle = NULL;

/* Inicializar la aplicación auxiliar.
 * Devuelve ESP_OK si se creó la tarea correctamente. */
esp_err_t aux_app_init(void)
{
    ESP_LOGI(TAG, "Inicializando aplicación auxiliar");
    return ESP_OK;
}

int aux_app_read_pir(void)
{
    return g_pir;
}

int aux_app_read_sw(void) 
{
    return g_sw;
}

esp_err_t aux_app_set_led(int on)
{
    /* gpio_set_level(...) */ 
    return ESP_OK;
}

/* Tarea principal de la aplicación auxiliar */
// aux_app.c (task de ejemplo)
void aux_app_poll_task(void *arg){
  for(;;){
    g_pir = aux_app_read_pir();
    g_sw  = aux_app_read_sw();
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