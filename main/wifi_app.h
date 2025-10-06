#ifndef WIFI_APP_H
#define WIFI_APP_H

// WiFi SoftAP
#define AP_SSID      "LM35_SLAVE_AP"
#define AP_PASS      "12345678"
#define AP_CHANNEL   1
#define AP_MAX_CONN  2

void wifi_app_init_softap(void);

#endif // WIFI_APP_H