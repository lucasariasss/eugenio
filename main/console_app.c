// console_app.c
#include "console_app.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#include "esp_log.h"

#include "msg_app.h"   // udp_sock, slave_addr, last_temp, setpoint_c

#define TAG "console_app: "

// Helpers
static void trim(char *s){
    // recorta espacios al inicio/fin y colapsa \r\n
    size_t n = strlen(s);
    while (n && (s[n-1]=='\n' || s[n-1]=='\r' || isspace((unsigned char)s[n-1]))) s[--n]=0;
    char *p = s; while (*p && isspace((unsigned char)*p)) p++;
    if (p!=s) memmove(s,p,strlen(p)+1);
}
static float parse_float(const char* s, int *ok){
    // acepta coma o punto decimal
    char buf[64]; strncpy(buf, s, sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    for (char *p=buf; *p; ++p) if (*p==',') *p='.';
    char *end=NULL;
    float v = strtof(buf, &end);
    *ok = (end!=buf);
    return v;
}

void console_app_task_print_5s(void *arg){
    while (1){
        const char *hello = "HELLO\n";
        sendto(udp_sock, hello, strlen(hello), 0,
        (struct sockaddr*)&slave_addr, sizeof(slave_addr));


        if (!isnan(last_temp)) ESP_LOGI(TAG, "[Maestro] Temp actual: %.2f C (SP=%.2f)", last_temp, setpoint_c);
        else                   ESP_LOGI(TAG, "[Maestro] Esperando TEMP... (SP=%.2f)", setpoint_c);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void console_app_task(void *arg){
    char line[128];
    while (1){
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
        trim(line);
        if (!*line) continue;

        if (strncmp(line, "set", 3)==0){
            const char *argp = line+3;
            while (isspace((unsigned char)*argp)) argp++;
            int ok=0; float v = parse_float(argp, &ok);
            if (!ok || !(v>0 && v<120)){
                ESP_LOGE(TAG, "Error: valor inválido. Rango: 0<SP<120");
                continue;
            }
            char out[32]; int len = snprintf(out, sizeof(out), "SET:%.2f\n", v);
            sendto(udp_sock, out, len, 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
            // Re-HELLO por si el esclavo reinició
            const char *hello = "HELLO\n";
            sendto(udp_sock, hello, strlen(hello), 0, (struct sockaddr*)&slave_addr, sizeof(slave_addr));
            setpoint_c = v;
            ESP_LOGI(TAG, "Setpoint %.2f C enviado.", v);
        }
        else {
            ESP_LOGE(TAG, "Comando no reconocido.");
        }
    }
}