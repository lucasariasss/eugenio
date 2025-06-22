/*
 * uart_app.h
 *
 *  Created on: 8 sep. 2024
 *      Author: arias
 */

#ifndef MAIN_UART_APP_H_
#define MAIN_UART_APP_H_

#include <cJSON.h>


esp_err_t uart_app_init(void);

void uart_app_send_message(int disease);


#endif /* MAIN_UARTHANDLE_H_ */
