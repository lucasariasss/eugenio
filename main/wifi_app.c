// wifi_app.c

#include "wifi_app.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include <string.h>
#include <stdbool.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"

#include "app_role.h"

#define TAG "wifi_app: "
#define WIFI_GOT_IP_BIT  BIT0

static void wifi_sta_got_ip_handler(void* arg,
                                    esp_event_base_t base,
                                    int32_t id,
                                    void* data)
{
    xEventGroupSetBits((EventGroupHandle_t)arg, WIFI_GOT_IP_BIT);
}

static void wifi_app_stack_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
#if MASTER == 1
    esp_netif_create_default_wifi_sta();
#elif THERMAL == 1
    esp_netif_create_default_wifi_ap();
#endif
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

static void wifi_app_config(wifi_mode_t mode, wifi_config_t *wifi_config) {
    if(mode != WIFI_MODE_STA && mode != WIFI_MODE_AP) {
        ESP_LOGE(TAG, "Modo WiFi inválido");
        return;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    ESP_ERROR_CHECK(esp_wifi_set_config((mode==WIFI_MODE_STA)? WIFI_IF_STA : WIFI_IF_AP,wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    if (mode == WIFI_MODE_STA) {
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
}

void wifi_app_init_softap(void) {
    wifi_app_stack_init();

    wifi_config_t wifi_config = { 0 };
    strcpy((char*)wifi_config.ap.ssid, AP_SSID);
    wifi_config.ap.ssid_len = strlen(AP_SSID);
    strcpy((char*)wifi_config.ap.password, AP_PASS);
    wifi_config.ap.channel = AP_CHANNEL;
    wifi_config.ap.max_connection = AP_MAX_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    if (strlen(AP_PASS) == 0) wifi_config.ap.authmode = WIFI_AUTH_OPEN;

    wifi_app_config(WIFI_MODE_AP, &wifi_config);

    ESP_LOGI(TAG, "SoftAP iniciado: SSID=%s, pass=%s, canal=%d",
             AP_SSID, AP_PASS, AP_CHANNEL);
}

void wifi_app_connect_sta(void){
    wifi_app_stack_init();

    wifi_config_t wifi_config = {0};
    strcpy((char*)wifi_config.sta.ssid, STA_SSID);
    strcpy((char*)wifi_config.sta.password, STA_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    wifi_app_config(WIFI_MODE_STA, &wifi_config);

    ESP_LOGI(TAG, "Conectando a %s ...", STA_SSID);
    
    EventGroupHandle_t eg = xEventGroupCreate();
    esp_event_handler_instance_t inst = NULL;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_sta_got_ip_handler, eg, &inst));

    esp_netif_t* sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    bool has_ip = false;
    if (sta) {
        esp_netif_ip_info_t ip;
        if (esp_netif_get_ip_info(sta, &ip) == ESP_OK && ip.ip.addr != 0) {
            has_ip = true;
        }
    }
    if (!has_ip) {
        xEventGroupWaitBits(eg, WIFI_GOT_IP_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(
        IP_EVENT, IP_EVENT_STA_GOT_IP, inst));
    vEventGroupDelete(eg);

    ESP_LOGI(TAG, "STA lista con IP.");
}