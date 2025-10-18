// app_role.h
#ifndef APP_ROLE_H
#define APP_ROLE_H

#define SELECTOR 0b01
#define MASTER   ((SELECTOR >> 0) & 1)
#define THERMAL  ((SELECTOR >> 1) & 1)

#if (THERMAL + MASTER) > 1
#error "Only one application role can be enabled at a time"
#elif SELECTOR == 0
#error "At least one application role must be enabled"
#endif

#endif // APP_ROLE_H