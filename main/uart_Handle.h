/*
 * uart_Handle.h
 *
 *  Created on: 8 sep. 2024
 *      Author: arias
 */

#ifndef MAIN_UART_HANDLE_H_
#define MAIN_UART_HANDLE_H_

#include <cJSON.h>


esp_err_t uart_Handle_init(void);

void uart_Handle_send_message(int disease);


#endif /* MAIN_UARTHANDLE_H_ */
