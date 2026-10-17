# FreeRTOS bring-up: two tasks

This is the first part of notes issue #9, not the complete task/ISR prototype:
https://github.com/immorcoding/notes/issues/9

## Review checkpoints

- LED baseline: `81b7e5afb21dd6b267069534d44c534e3a5e2ba5`.
- Imported kernel baseline: `6aba84371609f7bb1d2f771ed20c4c8b33204eca`.
- The two-task integration and this board evidence are captured together in
  the integration commit on `codex/f411-freertos-baseline`. The corresponding
  full commit SHA is recorded in notes issue #9.
- The imported `Source/` and existing clock configuration are unchanged.

## Execution

`main()` initializes HAL, clocks, and GPIO, then calls `app_start()`.
It creates these tasks and starts the scheduler:

| Task | Priority | Action | Block interval |
| --- | --- | --- | --- |
| led | 2 | Toggle PC13; increment led_count | 1000 ticks |
| heartbeat | 1 | Increment heartbeat_count | 100 ticks |

Both tasks have 256-word stacks (1024 bytes on this port). The kernel uses
`heap_4.c` with a 16 KiB heap. The scheduler also creates its idle task.
There is no software timer task, application ISR, or event recorder yet.

The configured tick rate is 1000 Hz. Tasks use `vTaskDelay`, not HAL busy
waiting. Delays are relative and subject to scheduling and tick phase;
the counters are progress indicators, not precision timing measurements.

## Exceptions and regeneration

The Cortex-M4F port provides SVC and PendSV directly. Their empty generated
handlers have been removed, and the matching handler-generation flags in
the `.ioc` are disabled. When regenerating with CubeMX, verify that these
handlers remain owned by the port and are not generated a second time.

The generated SysTick handler retains `HAL_IncTick()` and calls
`xPortSysTickHandler()` after the scheduler has started. This shared time
base requires the current 1000 Hz RTOS tick and disabled tickless idle.
Review the HAL time base again if either setting changes.

NVIC uses priority group 4 and four implemented priority bits. The RTOS
system-call threshold is configured as 5; port assertions check priority
configuration at runtime. No application FromISR call is validated by
this two-task bring-up.

## Validation and board handoff

The agent built Debug with the installed STM32 GNU 14.3.1 toolchain in
`build/agent-debug`, separate from the VS Code Debug build directory:

```powershell
# Make the installed STM32 toolchain's bin directory available on PATH.
cmake --preset Debug -B build/agent-debug
cmake --build build/agent-debug --parallel 4
```

Output: `build/agent-debug/STM32F411CEU6.elf`.
The link report uses 18240 bytes RAM and 12124 bytes Flash.
On 2026-10-17, after the board handoff below, the learner reported that
everything was normal. This is user-reported board validation of the LED
and task counters; no raw trace or timing measurement was supplied.

Before running, the learner should predict which task runs first and
whether the lower-priority heartbeat can run while the LED task is delayed.

For the usual VS Code workflow, open this firmware project's directory,
build the Debug preset, then launch its existing ST-Link configuration.
That configuration selects the VS Code build output, not necessarily
the agent's separate ELF. Stop at `main`, then continue execution.

Observe the LED and inspect `led_count` and
`heartbeat_count` after running and pausing. Resume and pause again
to check that both advance. Avoid breakpoints in SysTick while observing
normal timing. If execution stalls, inspect `g_lab_failure_file` and
`g_lab_failure_line` and the current call stack.

Before running, the learner predicted that the delayed LED task enters the
Blocked state and yields CPU time, allowing the lower-priority heartbeat
task to run. This was an answer to the supplied scenario, not an independent
assessment using a new scenario. The reported board outcome was consistent
with that prediction. The controlled ISR and event trace are subsequent work;
notes issue #9 and parent issue #6 are not complete.
