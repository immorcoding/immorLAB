#include "FreeRTOS.h"
#include "task.h"

/* Inspect these variables after pausing the debugger on an assertion. */
const char * volatile g_lab_failure_file;
volatile int g_lab_failure_line;

void lab_assert(const char *file, int line)
{
    __disable_irq();
    g_lab_failure_file = file;
    g_lab_failure_line = line;

    for (;;)
    {
        __NOP();
    }
}

void vApplicationMallocFailedHook(void)
{
    lab_assert(__FILE__, __LINE__);
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;
    lab_assert(__FILE__, __LINE__);
}
