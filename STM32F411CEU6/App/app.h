#ifndef APP_H
#define APP_H

#include <stdint.h>

extern volatile uint32_t led_count;
extern volatile uint32_t heartbeat_count;

void app_start(void);

#endif
