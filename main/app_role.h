// app_role.h
#ifndef APP_ROLE_H
#define APP_ROLE_H

#define SELECTOR 0b100
#define MASTER   ((SELECTOR >> 0) & 1) // 001
#define THERMAL  ((SELECTOR >> 1) & 1) // 010
#define AUX      ((SELECTOR >> 2) & 1) // 100

#if (MASTER + THERMAL + AUX) != 1
#error "Elegir un solo rol por build"
#endif // (MASTER + THERMAL + AUX) != 1

#if MASTER == 1
#warning "buildeando MASTER"
#elif THERMAL == 1
#warning "buildeando THERMAL"
#elif AUX == 1
#warning "buildeando AUX"
#endif // WARNING SWITCH

#if THERMAL == 1 || AUX == 1
#define SLAVE 1
#endif // THERMAL == 1 || AUX == 1

#endif // APP_ROLE_H