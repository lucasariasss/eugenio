#ifndef MSG_APP_H_
#define MSG_APP_H_

#include <stdbool.h>           
#include "freertos/FreeRTOS.h"    
#include "freertos/semphr.h"      
#include "lwip/sockets.h"         

#define UDP_PORT 3333
#define SLAVE_IP   "192.168.4.1"  // IP por defecto del SoftAP del ESP32

extern int udp_sock;
extern struct sockaddr_in master_addr;
extern struct sockaddr_in slave_addr;

extern bool master_known;
extern SemaphoreHandle_t sock_mutex;
extern float setpoint_c;
extern volatile float last_temp;

void msg_app_open_slave(void);
void msg_app_open_master(void);
void msg_app_task_rx_slave(void *arg);
void msg_app_task_rx_master(void *arg);

#endif // MSG_APP_H_