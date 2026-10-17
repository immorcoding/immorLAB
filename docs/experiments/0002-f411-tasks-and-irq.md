# Historical record: F411 tasks and software IRQ

Recorded on 2026-10-17 against board-tested commit
`eeddee910e6c4d7ea945e6fce0987ebeb200d177`. The following text preserves
the experiment and review at that checkpoint. Current baseline instructions
are in the root README and STM32F411CEU6/App/README.md.

---

# FreeRTOS bring-up

## Current stage: software-pended IRQ

**IRQ source: software sets the NVIC pending bit for the unused EXTI0 IRQ.
No GPIO edge or EXTI peripheral trigger is configured.** The CPU performs
real exception entry, ISR execution, and exception return. This experiment
validates ISR -> task notification -> scheduling, not the GPIO/EXTI hardware
event path. A software-pended IRQ is not a direct C call to the handler.

The LED task (priority 2) waits for a notification. After a 1000-tick delay,
the heartbeat task (priority 1) pends EXTI0 once, then continues its ordinary
100-tick heartbeat. The ISR notifies the LED task and requests a context
switch with `portYIELD_FROM_ISR`. IRQ priority is 6, below the configured
maximum syscall urgency (numerical threshold 5).

`INCLUDE_vTaskSuspend` is enabled so `portMAX_DELAY` means indefinite
notification waiting. The LED now toggles once per reset, instead of
blinking periodically, and then waits for another notification.

### Reading the trace

`trace` holds 16 entries with `event`, `tick`, and `value`. Read only
`trace[0]` through `trace[trace_count - 1]`, in array order. `tick` is the
HAL millisecond counter; multiple events can share a tick, so it is not
a high-resolution timing measurement.

| Event value | Name | Recorded at |
| --- | --- | --- |
| 1 | LED_WAIT | LED task before its first notification wait |
| 2 | TASK_BEFORE | Heartbeat task before pending the IRQ |
| 3 | IRQ_ENTER | Start of the ISR; value is irq_count |
| 4 | IRQ_NOTIFY | After notifying; value is the woken flag |
| 5 | IRQ_EXIT | Before portYIELD_FROM_ISR and the ISR return |
| 6 | LED_WAKE | LED task after notification and LED toggle |
| 7 | TASK_AFTER | Heartbeat task after pending the IRQ |

The numeric labels identify events, not a promise about their order.
Before running, the learner should predict the order, especially whether
LED_WAKE or TASK_AFTER occurs first, and explain when the task switch occurs.

The recorder briefly saves PRIMASK, masks configurable interrupts while
appending one entry, and restores the old mask. It preserves entry order
between task and ISR writers. This instrumentation changes timing and is
not evidence for latency bounds or BASEPRI protection boundaries.
When full, it increments `trace_overflow` rather than overwriting evidence.
No printf is used on the event path.

### Board handoff

1. Open the inner STM32F411CEU6 project, rebuild Debug, and launch ST-Link.
2. Start from a reset (not merely continue an old paused session). Continue
   from main, run for at least two seconds, then pause.
3. Inspect trace_count, trace_overflow, trace, irq_count, led_count, and
   heartbeat_count. Report the actual event sequence and IRQ_NOTIFY.value.
4. Continue and pause again: the heartbeat should advance, while the
   one-shot trace and IRQ/LED counters should remain unchanged.
5. Reset and repeat once to compare ordering. Avoid breakpoints in the ISR
   and trace recorder during this observation.

The structural checks are seven entries, no overflow, one IRQ, and one LED
toggle. Exact event order is to be predicted and compared with real board
evidence. If stopped in lab_assert, inspect g_lab_failure_file,
g_lab_failure_line, and the call stack.

Agent Debug build: STM32 GNU 14.3.1, RAM 18464 B, Flash 13680 B.
The strong EXTI0 vector is checked in the linked ELF.

### Board evidence and review (2026-10-17)

Before running, the learner predicted that the LED task would execute
before the heartbeat task continued after triggering the interrupt.
The agent then explained the expected scheduling path before observation;
this is guided learning evidence, not an unseen-scenario mastery assessment.

The learner reported these debugger values at the breakpoint after
record(TASK_AFTER, 0U), before the first heartbeat increment:

- heartbeat_count = 0
- trace_count = 7; trace_overflow = 0
- irq_count = 1; led_count = 1
- g_lab_failure_file = 0x0; g_lab_failure_line = 0

The pasted expanded trace was interpreted as follows:

| Index | Event | Tick | Value |
| --- | --- | --- | --- |
| 0 | 1: LED_WAIT | 0 | 0 |
| 1 | 2: TASK_BEFORE | 1000 | 0 |
| 2 | 3: IRQ_ENTER | 1000 | 1 |
| 3 | 4: IRQ_NOTIFY | 1000 | 1 |
| 4 | 5: IRQ_EXIT | 1000 | 0 |
| 5 | 6: LED_WAKE | 1000 | 1 |
| 6 | 7: TASK_AFTER | 1000 | 0 |

The measured entry order agrees with the learner's prediction: the ISR
wakes the higher-priority task, which executes before the interrupted
lower-priority task resumes its application code. IRQ_NOTIFY.value = 1
is the FromISR woken flag. Array position determines event order; equal
millisecond ticks do not establish precise execution time or latency.

The agent requested continue/pause and reset/repeat checks. The learner
then accepted the result and authorized completing the record and closing
the issues; no separate values or second-run trace were supplied, so those
checks are not recorded as independently observed repetitions.

Verdict: retain this small task/ISR prototype as the shared starting point
for subsequent concurrency experiments. The learner approved the minimal
structure, short names, explicit software IRQ origin, and final observed
result. Notes issues #9 and #6 are ready to close with the fixed commit SHA
and evidence links. The implementation remains on the prototype branch;
merging into main is a separate action.

This validates only the configured software-pended IRQ path. It does not
validate GPIO/EXTI hardware triggering, other interrupt priorities, nested
IRQs, BASEPRI boundary experiments, mutex inheritance, or precise clock
accuracy. The larger learning spec (notes #5) remains in progress.

## Historical two-task baseline

The following sections describe the previously committed and board-tested
two-task version. Its periodic LED behavior is superseded by the one-shot
software IRQ stage above.

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
with that prediction. At that checkpoint the controlled ISR and event
trace were subsequent work, and notes #9 and #6 were not complete. Their
later software IRQ evidence and review are recorded above.
