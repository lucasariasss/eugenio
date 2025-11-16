// aux_app.h

#ifndef AUX_APP_H_
#define AUX_APP_H_

#include "esp_err.h"

esp_err_t aux_app_init(void);
int aux_app_read_pir(void);
int aux_app_read_sw(void);
esp_err_t aux_app_set_led(int on);

void aux_app_led_task(void *arg);
void aux_app_poll_task(void *arg); 

#endif // AUX_APP_H