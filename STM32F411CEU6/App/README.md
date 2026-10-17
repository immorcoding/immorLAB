# F411 baseline application

`main()` initializes HAL, clocks, and GPIO, then calls `app_start()`.
The application stays in `app.c`, without additional product layers.

## Scenario

- LED task, priority 2: record LED_WAIT and block on a task notification.
- Heartbeat task, priority 1: delay 1000 ticks, record TASK_BEFORE, and
  pend the unused EXTI0 IRQ once. Afterward record TASK_AFTER, then increment
  heartbeat_count every 100 ticks.
- ISR, NVIC priority 6: record IRQ_ENTER, notify the LED task, record
  IRQ_NOTIFY and IRQ_EXIT, and request switching with portYIELD_FROM_ISR.
- LED task wakes, toggles PC13, records LED_WAKE, and waits again.

**The interrupt source is software setting NVIC pending. No GPIO edge or
EXTI peripheral trigger is configured.** Exception entry, ISR execution,
and return are real. This checks ISR -> notification -> scheduling only.

FreeRTOS V11.3.0 uses ARM_CM4F, heap_4 with a 16 KiB heap, and a 1000 Hz
tick. Task stacks are 256 words each; the idle task is also present.
NVIC has four preemption priority bits (group 4), syscall threshold 5.
INCLUDE_vTaskSuspend enables indefinite notification waits with portMAX_DELAY.

## Observe

Rebuild Debug and start ST-Link debugging from the inner firmware project.
Reset, continue from main, and break on the first heartbeat_count increment
after record(TASK_AFTER, 0U), or run for two seconds and pause.

Watch:

```text
trace_count
trace_overflow
trace
irq_count
led_count
heartbeat_count
g_lab_failure_file
g_lab_failure_line
```

Read trace[0] through trace[trace_count - 1]. Each entry contains event,
tick (HAL milliseconds), and value. The recorded baseline order is:

| Event | Name | Value |
| --- | --- | --- |
| 1 | LED_WAIT | 0 |
| 2 | TASK_BEFORE | 0 |
| 3 | IRQ_ENTER | IRQ count |
| 4 | IRQ_NOTIFY | FromISR woken flag (1) |
| 5 | IRQ_EXIT | 0 |
| 6 | LED_WAKE | LED count |
| 7 | TASK_AFTER | 0 |

Expect seven entries, no overflow, one IRQ, and one LED toggle. The
heartbeat counter is zero before its first increment and advances after
continuing. LED/IRQ counts and trace remain unchanged until another reset.
The LED toggles once per reset, rather than blinking periodically.

Array order gives event order; equal tick values do not measure exact
latency. The recorder briefly masks interrupts with PRIMASK and restores
the old mask. It is not evidence for BASEPRI boundaries or latency limits.
Full buffers increment trace_overflow instead of overwriting entries.

## Integration boundaries

- FreeRTOS Source is imported kernel code; project settings and hooks are
  in the adjacent Config directory.
- The port owns SVC and PendSV. Keep CubeMX generation of their empty
  handlers disabled when regenerating from the .ioc.
- SysTick increments HAL time and, after scheduler startup, RTOS time.
  Revisit this arrangement if changing the 1000 Hz tick or tickless setting.
- EXTI0_IRQHandler is in app.c and uses an otherwise unused vector. Check
  ownership before adding any real EXTI0 peripheral use.

Full historical predictions, observations, build versions, and limitations:
[task/IRQ evidence](../../docs/experiments/0002-f411-tasks-and-irq.md).
