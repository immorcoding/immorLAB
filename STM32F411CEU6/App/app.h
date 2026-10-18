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
    TASK_AFTER,
    CASE_BEGIN,
    TASK_READ,
    TASK_WRITE,
    IRQ_WRITE,
    CASE_DONE
};

typedef struct
{
    uint32_t event;
    uint32_t tick;
    uint32_t value;
} TraceEntry;

extern volatile TraceEntry trace[32];
extern volatile uint32_t trace_count;
extern volatile uint32_t trace_overflow;

typedef struct
{
    uint32_t count;
    uint32_t updates;
    uint32_t task_temp;
    uint32_t irq_temp;
} CaseResult;

extern volatile uint32_t count;
extern volatile uint32_t updates;
extern volatile uint32_t task_temp;
extern volatile uint32_t irq_temp;
extern volatile uint32_t experiment_done;
extern volatile CaseResult results[2];

void app_start(void);

#endif
