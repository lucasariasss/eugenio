#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "uart_app.h"
#include "wifi_app.h"

static const char TAG[] = "main";

void wifi_application_connected_events(void)
{
	ESP_LOGI(TAG, "Aplicacion WiFi conectada");
}

void app_main(void)
{
    // Initialize NVS
	esp_err_t ret = ESP_OK;
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Inicia Wifi
	wifi_app_start();

	// Configurar el callback para eventos de conexión
	wifi_app_set_callback(&wifi_application_connected_events);

    ESP_LOGI(TAG, "Inicializando UART...");
    
    // Llama a la función de inicialización de UART
    esp_err_t uart_ret = uart_Handle_init();
    if (uart_ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Error al inicializar UART: %s", esp_err_to_name(uart_ret));
    }
    else
    {
        ESP_LOGI(TAG, "UART inicializada correctamente.");
    }
 	ESP_ERROR_CHECK(ret);
}


