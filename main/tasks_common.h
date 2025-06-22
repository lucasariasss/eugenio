/*
 * tasks_common.h
 *
 *  Created on: Sep 26, 2023
 *      Author: arias
 */

#ifndef MAIN_TASKS_COMMON_H_
#define MAIN_TASKS_COMMON_H_

//tasks de la app de wifi
#define WIFI_APP_TASK_STACK_SIZE			4096
#define WIFI_APP_TASK_PRIORITY				5
#define WIFI_APP_TASK_CORE_ID				0

//informacion de las tareas del servidor http
#define HTTP_SERVER_TASK_STACK_SIZE 		8192
#define HTTP_SERVER_TASK_PRIORITY			4
#define HTTP_SERVER_TASK_CORE_ID			0

//Monitoreo del servidor http
#define HTTP_SERVER_MONITOR_STACK_SIZE 		4096
#define HTTP_SERVER_MONITOR_PRIORITY		3
#define HTTP_SERVER_MONITOR_CORE_ID			0

//tasks de el controlador de uart
#define BOOTLOADER_UART                     0
#define UART_APP_TASK_STACK_SIZE			1024
#define UART_APP_TASK_PRIORITY           5
#define UART_APP_TASK_CORE_ID			0

//tasks de la aplicacion de uart
#if defined(BOOTLOADER_UART) && (BOOTLOADER_UART == 1)
#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)
#define UART_NUM (UART_NUM_0)
#else
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define UART_NUM (UART_NUM_1)
#endif

//tasks de la aplicacion de lcd
#define SCL_PIN (GPIO_NUM_22)
#define SDA_PIN (GPIO_NUM_21)
#define I2C_NUM (I2C_NUM_1)
#define LCD_ADDRESS 0x3C

#endif /* MAIN_TASKS_COMMON_H_ */
