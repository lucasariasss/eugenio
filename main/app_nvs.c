/*
 * app_nvs.c
 *
 *  Created on: Apr 16, 2024
 *      Author: arias
 */

#ifdef HAS_STA_MODE == 1

#include  <stdbool.h>
#include  <stdio.h>
#include  <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "app_nvs.h"

// Tag for logging the monitor
static const char TAG[] = "nvs";

typedef struct {
    char ssid[MAX_SSID_LENGTH];
    char password[MAX_PASSWORD_LENGTH];
    uint16_t use_count;   // número de veces que se usó esta credencial
} wifi_cred_t;

#define MAX_SAVED_CREDS 5

wifi_cred_t saved_creds[MAX_SAVED_CREDS];

// NVS name space used for station mode credentials
const char app_nvs_sta_creds_namespace[] = "stacreds";

esp_err_t app_nvs_save_sta_creds(void)
{
	nvs_handle handle;
	esp_err_t esp_err;
	ESP_LOGI(TAG, "app_nvs_save_sta_creds: Salvando credenciales de modo estación en NVS");

	wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();
	wifi_cred_t new_cred;

	uint8_t index = 99; // Valor por defecto para indicar que no se encontró un espacio vacío

	strcpy(new_cred.ssid, (const char *)wifi_sta_config->sta.ssid);
	strcpy(new_cred.password, (const char *)wifi_sta_config->sta.password);

	for(uint8_t i = 0; i < MAX_SAVED_CREDS; i++)
	{
		if (saved_creds[i].ssid[0] == '\0') // Encontrar primer espacio vacío
		{
			index = i;
			break;
		}
	}

	if (index == 99) // Si no se encontró un espacio vacío, reemplazar la menos usada
	{
		uint16_t min_use_count = 65535; // Valor máximo para uint16_t
		for(uint8_t i = 0; i < MAX_SAVED_CREDS; i++)
		{
			if (saved_creds[i].use_count < min_use_count)
			{
				min_use_count = saved_creds[i].use_count;
				index = i;
			}
		}
	}



	if (wifi_sta_config)
	{
		esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
		if (esp_err != ESP_OK)
		{
			printf("app_nvs_save_sta_creds: Error (%s) abriendo NVS handle\n", esp_err_to_name(esp_err));
			return esp_err;
		}

		// Establecer credenciales
		esp_err = nvs_set_blob(handle, "saved_creds", saved_creds, MAX_SSID_LENGTH);
		if (esp_err != ESP_OK)
		{
			printf("app_nvs_save_sta_creds: Error (%s) estableciendo credenciales a NVS\n", esp_err_to_name(esp_err));
			return esp_err;
		}

		// Confirmar credenciales en NVS
		esp_err = nvs_commit(handle);
		if (esp_err != ESP_OK)
		{
			printf("app_nvs_save_sta_creds: Error (%s) comitting credentials to NVS\n", esp_err_to_name(esp_err));
			return esp_err;
		}
		nvs_close(handle);
		ESP_LOGI(TAG, "app_nvs_save_sta_creds: wrote wifi_sta_config: Station SSID: %s Password: %s", wifi_sta_config->sta.ssid, wifi_sta_config->sta.password);
	}

	printf("app_nvs_save_stacreds: returned ESP_OK\n");
	return ESP_OK;
}

bool app_nvs_load_sta_creds(void)
{
	nvs_handle handle;
	esp_err_t esp_err;

	ESP_LOGI(TAG, "app_nvs_load_sta_creds: Cargando credenciales de modo estación desde NVS");

	esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READONLY, &handle);
	if (esp_err != ESP_OK)
	{
		printf("app_nvs_load_sta_creds: Error (%s) abriendo NVS handle!\n", esp_err_to_name(esp_err));
		return false;
	}

	size_t size = sizeof(saved_creds);
	esp_err = nvs_get_blob(handle, "saved_creds", saved_creds, &size);
	nvs_close(handle);

	if (esp_err != ESP_OK)
	{
		ESP_LOGE(TAG, "No se pudieron leer credenciales: %s", esp_err_to_name(esp_err));
		return false;
	}

	wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();
	if (wifi_sta_config == NULL)
	{
		wifi_sta_config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
	}
	memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));

	// Buscar primer credencial válida
	app_nvs_print_sta_creds();
	return false;
}

esp_err_t app_nvs_clear_sta_creds(void)
{
	nvs_handle handle;
	esp_err_t esp_err;
	ESP_LOGI(TAG, "app_nvs_clear_sta_creds: Clearing Wifi station mode credentials from flash");

	esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
	if (esp_err != ESP_OK)
	{
		printf("app_nvs_clear_sta_creds: Error (%s) opening NVS handle!\n", esp_err_to_name(esp_err));
		return esp_err;
	}

	// Erase credentials
	esp_err = nvs_erase_all(handle);
	if (esp_err != ESP_OK)
	{
		printf("app_nvs_clear_sta_creds: Error (%s) erasing station mode credentials!\n", esp_err_to_name(esp_err));
		return esp_err;
	}

	// Commit clearing credentials to NVS
	esp_err = nvs_commit(handle);
	if (esp_err != ESP_OK)
	{
		printf("app_nvs_clear_sta_creds: Error (%s) NVS commit!\n", esp_err_to_name(esp_err));
		return esp_err;
	}
	nvs_close(handle);

	printf("app_nvs_clear_sta_creds: returned ESP_OK\n");
	return ESP_OK;
}

void app_nvs_print_sta_creds(void)
{
	nvs_handle handle;
	esp_err_t esp_err;

	ESP_LOGI(TAG, "app_nvs_print_sta_creds: Printing saved Wifi station mode credentials");

	if (nvs_open(app_nvs_sta_creds_namespace, NVS_READONLY, &handle) == ESP_OK)
	{
		size_t size = sizeof(saved_creds);
		esp_err = nvs_get_blob(handle, "saved_creds", saved_creds, &size);
		if (esp_err != ESP_OK)
		{
			printf("app_nvs_print_sta_creds: Error (%s) retrieving saved credentials\n", esp_err_to_name(esp_err));
			nvs_close(handle);
			return;
		}

		for (uint8_t i = 0; i < MAX_SAVED_CREDS; i++)
		{
			printf("SSID: %s, Password: %s, Use Count: %d\n", saved_creds[i].ssid, saved_creds[i].password, saved_creds[i].use_count);
		}
		nvs_close(handle);
	}
	else
	{
		printf("app_nvs_print_sta_creds: No saved credentials found\n");
	}
}	

void app_nvs_struct_array_to_json( cJSON *json)
{
    for (int i = 0; i < MAX_SAVED_CREDS; i++) {
        if (saved_creds[i].ssid[0] != '\0') {     // Verifica si la credencial es válida
            cJSON *cred_json = cJSON_CreateObject();
            if (cred_json == NULL) {
                ESP_LOGE(TAG, "No se pudo crear JSON para credencial %d", i);
                continue;
            }

            cJSON_AddStringToObject(cred_json, "ssid", saved_creds[i].ssid);
            cJSON_AddStringToObject(cred_json, "password", saved_creds[i].password);
            cJSON_AddNumberToObject(cred_json, "use_count", saved_creds[i].use_count);

            // Se agrega cada objeto creado al arreglo JSON
            cJSON_AddItemToArray(json, cred_json);
        } else {
            ESP_LOGE(TAG, "No hay credencial válida en espacio %d", i);
        }
    }
}

#endif // HAS_STA_MODE

























