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

#include "msg_app.h"   // udp_sock, slave_addr, last_temp, g_setpoint

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

void console_app_task(void *arg){
    char line[128];
    while (1){
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }
        trim(line);
        if (!*line) continue;

        if (!strncmp(line, "set ", 4)) {
            int ok = 0;
            float sp = parse_float(line+4, &ok);
            if (!ok) { ESP_LOGE(TAG, "SET inválido"); continue; }
            char out[32];
            int len = snprintf(out, sizeof(out), "SET:%.2f\n", sp);
            if(len > 0)
            {
                if (msg_app_tx_to_thermal(out) < 0) ESP_LOGE(TAG,"Thermal no conocido");
            }
            continue;
        }
        else if (!strncmp(line, "cfg cooler ", 11)) {
            char *m = line + 11;
            for (char *p=m; *p; ++p) *p = (char)tolower((unsigned char)*p);
            if (!strncmp(m,"temp",4)   && m[4]=='\0') msg_app_tx_to_thermal("CFG:COOLER_SRC=TEMP\n");
            else if (!strncmp(m,"pir",3)    && m[3]=='\0') msg_app_tx_to_thermal("CFG:COOLER_SRC=PIR\n");
            else if (!strncmp(m,"switch",6) && m[6]=='\0') msg_app_tx_to_thermal("CFG:COOLER_SRC=SWITCH\n");
            else if (!strncmp(m,"off",3)    && m[3]=='\0') msg_app_tx_to_thermal("CFG:COOLER_SRC=OFF\n");
            else ESP_LOGE(TAG, "Uso: cfg cooler {temp|pir|switch|off}");
            continue;
        }
        else if (!strncmp(line, "cfg led ", 8)) {
            char *m = line + 8;
            for (char *p=m; *p; ++p) *p = (char)tolower((unsigned char)*p);
            if      (!strncmp(m,"pir",3)    && m[3]=='\0') msg_app_tx_to_aux("CFG:LED_SRC=PIR\n");
            else if (!strncmp(m,"switch",6) && m[6]=='\0') msg_app_tx_to_aux("CFG:LED_SRC=SWITCH\n");
            else if (!strncmp(m,"console",7)&& m[7]=='\0') msg_app_tx_to_aux("CFG:LED_SRC=CONSOLE\n");
            else ESP_LOGE(TAG, "Uso: cfg led {pir|switch|console}");
            continue;
        }
        else if (!strncmp(line, "led ", 4)) {
            const char *v = line + 4;
            if      (!strncmp(v,"0",1) && v[1]=='\0') msg_app_tx_to_aux("CMD:LED=0\n");
            else if (!strncmp(v,"1",1) && v[1]=='\0') msg_app_tx_to_aux("CMD:LED=1\n");
            else ESP_LOGE(TAG, "Uso: led {0|1}");
            continue;
        }
        else if (strcmp(line, "t") == 0) {
            if (!isnan(last_temp))
                ESP_LOGI(TAG, "[Maestro] Temp actual: %.2f C (SP=%.2f)", last_temp, g_setpoint);
            else
                ESP_LOGI(TAG, "[Maestro] Esperando TEMP... (SP=%.2f)", g_setpoint);
        }
        else {
            ESP_LOGE(TAG, "Comando no reconocido. (%s)", line);
        }
    }
}
/*
cfg led console
cfg led switch
cfg led pir
led 1
led 0
*/