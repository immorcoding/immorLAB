#include "app.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

volatile uint32_t led_count;
volatile uint32_t heartbeat_count;

static void LedTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        ++led_count;
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

static void HeartbeatTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        ++heartbeat_count;
        vTaskDelay(pdMS_TO_TICKS(100U));
    }
}

void app_start(void)
{
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    BaseType_t result = xTaskCreate(LedTask, "led", 256U, NULL, 2U, NULL);
    configASSERT(result == pdPASS);
    result = xTaskCreate(HeartbeatTask, "heartbeat", 256U, NULL, 1U, NULL);
    configASSERT(result == pdPASS);

    vTaskStartScheduler();
    configASSERT(0);
    for (;;)
    {
    }
}
