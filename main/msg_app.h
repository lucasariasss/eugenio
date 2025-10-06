#ifndef MSG_APP_H_
#define MSG_APP_H_

#include <stdbool.h>           
#include "freertos/FreeRTOS.h"    
#include "freertos/semphr.h"      
#include "lwip/sockets.h"         

#define UDP_PORT 3333

extern int udp_sock;
extern struct sockaddr_in master_addr;
extern bool master_known;
extern SemaphoreHandle_t sock_mutex;
extern float setpoint_c;

void msg_app_open_slave(void);
void msg_app_task_rx_slave(void *arg);

#endif // MSG_APP_H_