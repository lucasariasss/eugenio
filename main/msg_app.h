// msg_app.h

#ifndef MSG_APP_H_
#define MSG_APP_H_

#include <stdbool.h>           
#include "freertos/FreeRTOS.h"  
#include "lwip/sockets.h" 

#define UDP_PORT 3333
#define MASTER_IP "192.168.4.1"   // IP por defecto del SoftAP del maestro

extern int udp_sock;
extern struct sockaddr_in master_addr;
extern struct sockaddr_in slave_addr;

extern volatile bool master_known;
extern volatile bool slave_known;
extern float setpoint_c;
extern volatile float last_temp;

void msg_app_open_slave(void);
void msg_app_open_master(void);
void msg_app_task_rx(void *arg);

#endif // MSG_APP_H_