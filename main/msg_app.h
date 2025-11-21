// msg_app.h

#ifndef MSG_APP_H_
#define MSG_APP_H_

#include <stdbool.h>           
#include "freertos/FreeRTOS.h"  
#include "lwip/sockets.h" 

#define UDP_PORT 3333
#define MASTER_IP "192.168.4.1"   // IP por defecto del SoftAP del maestro

typedef enum 
{ 
    COOL_SRC_TEMP,
    COOL_SRC_PIR,
    COOL_SRC_SWITCH,
    COOL_SRC_OFF
} cool_src_t;
typedef enum
{
    LED_SRC_PIR,
    LED_SRC_SWITCH,
    LED_SRC_CONSOLE
} led_src_t;

extern int udp_sock;
extern struct sockaddr_in master_addr;
extern struct sockaddr_in slave_addr_thermal;
extern struct sockaddr_in slave_addr_aux;

extern volatile bool       master_known;
extern volatile bool       thermal_known;
extern volatile bool       aux_known;
extern volatile float      last_temp;
extern volatile cool_src_t g_cool_src;
extern volatile led_src_t  g_led_src;
extern volatile float      g_setpoint;
extern volatile int        g_cmd_led;   // 0/1 solo si LED_SRC_CONSOLE
extern volatile int        g_sw;        // 0/1 (estado último recibido o leído)
extern volatile int        g_pir;       // 0/1 (estado último leído)

// + helpers de TX específicos
int msg_app_tx_to_thermal(const char *s);
int msg_app_tx_to_aux(const char *s);
void msg_app_open_slave(void);
void msg_app_open_master(void);
void msg_app_task_rx(void *arg);

#endif // MSG_APP_H_