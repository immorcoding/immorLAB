#ifndef APP_H
#define APP_H

#include <stdint.h>

extern volatile uint32_t led_count;
extern volatile uint32_t heartbeat_count;
extern volatile uint32_t irq_count;

enum
{
    LED_WAIT = 1,
    TASK_BEFORE,
    IRQ_ENTER,
    IRQ_NOTIFY,
    IRQ_EXIT,
    LED_WAKE,
    TASK_AFTER
};

typedef struct
{
    uint32_t event;
    uint32_t tick;
    uint32_t value;
} TraceEntry;

extern volatile TraceEntry trace[16];
extern volatile uint32_t trace_count;
extern volatile uint32_t trace_overflow;

void app_start(void);

#endif
