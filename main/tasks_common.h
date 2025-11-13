/**
 * @file tasks_common.h
 * @author Lucas Arias (1605137@ucc.edu.ar)
 * @brief Declaraciones y definiciones comunes para la gestión de tareas en el proyecto.
 *
 * Este archivo contiene las declaraciones, macros y definiciones que son utilizadas
 * de manera común por las diferentes tareas del proyecto. Facilita la reutilización
 * de código y la organización de las tareas en el sistema.
 *
 * @date 2023-09-26
 */

#ifndef MAIN_TASKS_COMMON_H_
#define MAIN_TASKS_COMMON_H_

#define HAS_STA_MODE  0 // 0 = sin modo estación, 1 = con modo estación

#if HAS_STA_MODE == 0
#define INDEX_START "_binary_index_html_start"
#define INDEX_END   "_binary_index_html_end"
#define CSS_START   "_binary_app_css_start"
#define CSS_END     "_binary_app_css_end"
#define JS_START    "_binary_app_js_start"
#define JS_END      "_binary_app_js_end"
#elif HAS_STA_MODE == 1
#define INDEX_START "_binary_index_sta_html_start"
#define INDEX_END   "_binary_index_sta_html_end"
#define CSS_START   "_binary_app_sta_css_start"
#define CSS_END     "_binary_app_sta_css_end"
#define JS_START    "_binary_app_sta_js_start"
#define JS_END      "_binary_app_sta_js_end"
#else
#error "HAS_STA_MODE debe ser 0 (AP) o 1 (STA)"
#endif

//tasks de la app de wifi
#define WIFI_APP_TASK_STACK_SIZE			4096
#define WIFI_APP_TASK_PRIORITY				2
#define WIFI_APP_TASK_CORE_ID				0

//informacion de las tareas del servidor http
#define HTTP_SERVER_TASK_STACK_SIZE 		8192
#define HTTP_SERVER_TASK_PRIORITY			4
#define HTTP_SERVER_TASK_CORE_ID			0

//Monitoreo del servidor http
#define HTTP_SERVER_MONITOR_STACK_SIZE 		4096
#define HTTP_SERVER_MONITOR_PRIORITY		3
#define HTTP_SERVER_MONITOR_CORE_ID			0

//Controlador de uart
#define BOOTLOADER_UART                     0
#define UART_APP_TASK_STACK_SIZE			1024

//Definicion de la aplicacion de uart
#if defined(BOOTLOADER_UART) && (BOOTLOADER_UART == 1)
#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)
#define UART_NUM (UART_NUM_0)
#else
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define UART_NUM (UART_NUM_1)
#endif

#endif /* MAIN_TASKS_COMMON_H_ */
