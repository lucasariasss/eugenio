/*
 * app_nvs.h
 *
 *  Created on: Apr 16, 2024
 *      Author: arias
 */

#ifndef MAIN_APP_NVS_H_
#define MAIN_APP_NVS_H_

#if HAS_STA_MODE == 1

#include <cJSON.h>
#include "wifi_app.h"

/*
 * Guarda las credenciales WiFi en modo estación en NVS
 * @return ESP_OK si se realiza correctamente
 */
esp_err_t app_nvs_save_sta_creds(void);

/*
 * Carga las credenciales previamente guardadas desde NVS
 * @return true si se encontraron credenciales guardadas previamente
 */
bool app_nvs_load_sta_creds(void);

/*
 * Limpia las credenciales de modo estación del NVS
 * @return ESP_OK si se realiza correctamente
 */
esp_err_t app_nvs_clear_sta_creds(void);

/**
 * @brief Imprime las credenciales Wi-Fi de estación almacenadas en NVS.
 *
 * Esta función recupera y muestra el SSID y la contraseña Wi-Fi
 * guardados en la memoria No Volátil (NVS) para fines de depuración
 * o verificación. Está destinada para uso durante el desarrollo y
 * debe usarse con precaución para evitar exponer información sensible.
 */
void app_nvs_print_sta_creds(void);

/**
 * @brief Convierte un arreglo de estructuras wifi_cred_t a un objeto JSON.
 *
 * Esta función toma un arreglo de credenciales Wi-Fi y llena el objeto
 * cJSON proporcionado con sus datos, típicamente para serialización o almacenamiento.
 *
 * @param[in]  cred  Un arreglo de 5 estructuras wifi_cred_t con las credenciales Wi-Fi.
 * @param[out] json  Un puntero a un objeto cJSON donde se agregarán las credenciales.
 */
void app_nvs_struct_array_to_json(cJSON *json);

#endif // HAS_STA_MODE

#endif /* MAIN_APP_NVS_H_ */
