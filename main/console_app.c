// console_app.c

#include <math.h>
#include <string.h>

#include "console_app.h"
#include "msg_app.h"


// Muestra cada 5 s
void console_app_task_print_5s(void *arg){
    while (1){
        if (!isnan(last_temp)) printf("[Maestro] Temp actual: %.2f C\n", last_temp);
        else printf("[Maestro] Esperando TEMP...\n");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

// Consola: "set <valor>"
void console_app_task(void *arg){
    char line[64];
    while (1){
        fflush(stdout);
        if (fgets(line, sizeof(line), stdin)){
            if (strncmp(line,"set",3)==0){
                float v = atof(line+3);
                char out[32];
                int len = snprintf(out, sizeof(out), "SET:%.2f\n", v);
                sendto(udp_sock, out, len, 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
                // re-envía HELLO por si reinició el esclavo
                const char *hello = "HELLO\n";
                sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
                printf("Setpoint %.2f C enviado.\n", v);
            } else {
                printf("Comando no reconocido. Usa: set <valor>\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}