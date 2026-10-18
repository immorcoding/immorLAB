#include "app.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

volatile uint32_t led_count;
volatile uint32_t heartbeat_count;
volatile uint32_t irq_count;
volatile TraceEntry trace[32];
volatile uint32_t trace_count;
volatile uint32_t trace_overflow;
volatile uint32_t count;
volatile uint32_t updates;
volatile uint32_t task_temp;
volatile uint32_t irq_temp;
volatile uint32_t experiment_done;
volatile CaseResult results[2];

static TaskHandle_t led_task;

static void record(uint32_t event, uint32_t value)
{
    /* Keep task/ISR writes together; restore the caller's interrupt mask. */
    uint32_t mask = __get_PRIMASK();
    __disable_irq();

    if (trace_count < (sizeof(trace) / sizeof(trace[0])))
    {
        trace[trace_count].event = event;
        trace[trace_count].tick = HAL_GetTick();
        trace[trace_count].value = value;
        ++trace_count;
    }
    else
    {
        ++trace_overflow;
    }

    __set_PRIMASK(mask);
}

static void LedTask(void *argument)
{
    (void)argument;

    record(LED_WAIT, 0U);

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        ++led_count;
        record(LED_WAKE, led_count);
    }
}

static void RunCase(uint32_t protected)
{
    uint32_t previous_irqs = irq_count;

    count = 0U;
    updates = 0U;
    task_temp = 0U;
    irq_temp = 0U;
    record(CASE_BEGIN, protected);

    if (protected != 0U)
    {
        taskENTER_CRITICAL();
    }

    /* Deliberately split the operation; not a claim about count++ assembly. */
    task_temp = count;
    record(TASK_READ, task_temp);
    NVIC_SetPendingIRQ(EXTI0_IRQn);
    __DSB();
    __ISB();

    if (protected == 0U)
    {
        /* Reject a run if the intended injection point was not observed. */
        configASSERT(irq_count == previous_irqs + 1U);
    }
    else
    {
        configASSERT(irq_count == previous_irqs);
    }

    task_temp = task_temp + 1U;
    count = task_temp;
    ++updates;
    record(TASK_WRITE, count);

    if (protected != 0U)
    {
        taskEXIT_CRITICAL();
        __DSB();
        __ISB();
    }

    configASSERT(irq_count == previous_irqs + 1U);
    results[protected].count = count;
    results[protected].updates = updates;
    results[protected].task_temp = task_temp;
    results[protected].irq_temp = irq_temp;
    record(CASE_DONE, count);
}

static void HeartbeatTask(void *argument)
{
    (void)argument;

    vTaskDelay(pdMS_TO_TICKS(1000U));
    RunCase(0U);
    RunCase(1U);
    experiment_done = 1U;

    for (;;)
    {
        ++heartbeat_count;
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}

void EXTI0_IRQHandler(void)
{
    BaseType_t woken = pdFALSE;

    ++irq_count;
    record(IRQ_ENTER, irq_count);
    irq_temp = count;
    irq_temp = irq_temp + 1U;
    count = irq_temp;
    ++updates;
    record(IRQ_WRITE, count);
    vTaskNotifyGiveFromISR(led_task, &woken);
    record(IRQ_NOTIFY, (uint32_t)woken);
    record(IRQ_EXIT, 0U);
    portYIELD_FROM_ISR(woken);
}

void app_start(void)
{
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    BaseType_t result = xTaskCreate(LedTask, "led", 256U, NULL, 2U, &led_task);
    configASSERT(result == pdPASS);
    result = xTaskCreate(HeartbeatTask, "heartbeat", 256U, NULL, 1U, NULL);
    configASSERT(result == pdPASS);

    /* Borrow this unused vector; the NVIC performs real exception entry. */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 6U, 0U);
    HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    vTaskStartScheduler();
    configASSERT(0);
    for (;;)
    {
    }
}
