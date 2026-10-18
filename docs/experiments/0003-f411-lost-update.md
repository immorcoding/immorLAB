# F411 lost update: controlled task/ISR comparison

Learning task: https://github.com/immorcoding/notes/issues/10

## Status and scope

Branch: `lab/lost-update`, starting at `f411-baseline-v1`.
Base commit: `88e7b4e270ed55631d89fdb69605774d71f93a1c`.
Board comparison was validated in this session (2026-10-18): screenshots
show the first run; the learner reported matching results after reset/repeat.
The fixed experiment commit is linked from notes issue #10. The base SHA
above identifies the starting point, not this modified experiment.
The shared baseline and third-party kernel are unchanged.

This one-shot experiment runs two cases after a 1000-tick delay:

1. No protection: the task reads count, pends EXTI0, then increments its
   saved copy and writes it back after the ISR has completed.
2. Task critical section: the same IRQ request is made inside the complete
   task read/modify/write operation. Priority 6 is masked by this port's
   BASEPRI threshold of 5, so the IRQ should execute after critical exit.

Both cases reset count and updates to zero. The task and ISR each perform
one update. `updates` counts completed updates, independently of count;
its increments cannot overlap in this specific arranged sequence. It is
not a general-purpose synchronized counter and does not measure a payload.
Assertions reject a run if the IRQ is not serviced at the intended point.

The ISR still notifies the higher-priority LED task. This may run before
the Heartbeat task resumes, but does not access count or updates.
There are no delays or blocking calls inside the critical section.

## Predictions and learning checkpoint

During guided chat, the learner identified count as shared state, then
distinguished the task and ISR (accessors) from their private saved copies
after feedback. The learner correctly predicted a final value of 1 when
the ISR interrupts before task writeback, and 2 for non-overlapping updates.
They also correctly explained that suspending scheduling does not prevent
ISR/task races. These initial answers were part of guided teaching, not
an independent unseen-scenario assessment.

Screenshot-observed results, matching the predictions:

| Debugger expression | No protection: results[0] | Critical: results[1] |
| --- | --- | --- |
| count | 1 | 2 |
| updates | 2 | 2 |
| task_temp | 1 | 1 |
| irq_temp | 1 | 2 |

## Board evidence and review (2026-10-18)

The learner supplied expanded VS Code Watch screenshots showing both results
above, experiment_done=1, irq_count=2, led_count=2, trace_count=19, and
trace_overflow=0. The visible trace entries establish the decisive ordering:

| Index | Event | Tick | Value |
| --- | --- | --- | --- |
| 1 | CASE_BEGIN (8) | 1000 | 0 |
| 2 | TASK_READ (9) | 1000 | 0 |
| 3 | IRQ_ENTER (3) | 1000 | 1 |
| 4 | IRQ_WRITE (11) | 1000 | 1 |
| 5 | IRQ_NOTIFY (4) | 1000 | 1 |
| 6 | IRQ_EXIT (5) | 1000 | 0 |
| 7 | LED_WAKE (6) | 1000 | 1 |
| 8 | TASK_WRITE (10) | 1000 | 1 |
| 9 | CASE_DONE (12) | 1000 | 1 |
| 10 | CASE_BEGIN (8) | 1000 | 1 |
| 11 | TASK_READ (9) | 1000 | 0 |
| 12 | TASK_WRITE (10) | 1000 | 1 |
| 13 | IRQ_ENTER (3) | 1000 | 2 |
| 14 | IRQ_WRITE (11) | 1000 | 2 |
| 15 | IRQ_NOTIFY (4) | 1000 | 1 |
| 16 | IRQ_EXIT (5) | 1000 | 0 |
| 17 | LED_WAKE (6) | 1000 | 2 |
| 18 | CASE_DONE (12) | 1000 | 2 |

Index 0's event field was clipped in the supplied image, so it is not
transcribed as observed. Array order, not equal millisecond ticks, proves
the recorded order. Screenshots were supplied in chat; this repository
contains a transcription, not original image attachments or a raw export.
Failure-file/line values were not supplied and are not claimed as observed.

After being asked to reset and repeat, checking both count/updates results
and trace_count/overflow, the learner replied that results were consistent.
This is a user-reported repeat, not a second independently supplied trace.

The learner explained that pending an IRQ inside the critical section delays
its execution until unmasking, and that the complete read/modify/write must
be protected. Wording was refined to distinguish a protected compound
operation from a literal single machine instruction, and to limit the
claim to interrupts covered by this port's threshold.

For a new split-critical-section variant (read protected separately from
write), the learner independently proposed the ISR running after temp += 1
and before writeback. The agent then expanded that answer into the numeric
execution sequence. This supports identifying a new bad interleaving after
instruction, not an unprompted full explanation of every acceptance item.

Conclusion: the controlled unprotected case loses one update, while the
critical-section case preserves both updates for these accessors/configuration.
The learner's explanation of volatile versus synchronization has not yet
been separately checked. Issue #10 remains open; no full-spec completion
or broader mastery is claimed.

## Board handoff

Open `STM32F411CEU6/` in VS Code, build Debug, and launch ST-Link with the
newly built Debug ELF. Reset and run for at least two seconds, then pause.
Do not place breakpoints inside the ISR, critical section, or recorder.

Inspect and report:

- `experiment_done` (expected 1), `results[0]`, `results[1]`;
- `irq_count` and `led_count` (expected 2 each);
- `trace_count` (expected 19), `trace_overflow` (expected 0);
- expanded `trace[0]` through `trace[trace_count - 1]`;
- `g_lab_failure_file` and `g_lab_failure_line` (expected zero).

Reset and repeat, reporting a second result rather than assuming repeatability.
If stopped in lab_assert, report the failure file, line, and call stack.

Trace events retain baseline numbers; added events are:

| Value | Event | Recorded value |
| --- | --- | --- |
| 8 | CASE_BEGIN | 0: unprotected, 1: critical |
| 9 | TASK_READ | saved task copy |
| 10 | TASK_WRITE | count after task write |
| 11 | IRQ_WRITE | count after ISR write |
| 12 | CASE_DONE | final count |

For case 0, IRQ_WRITE should precede TASK_WRITE; for case 1, TASK_WRITE
should precede IRQ_WRITE. Read array order, not equal millisecond ticks.

## Configuration and limits

Agent Debug build passed with STM32 GNU 14.3.1: RAM 18712 B, Flash
14060 B. Commands from the firmware directory, with that toolchain on PATH:

```powershell
cmake --preset Debug -B build/agent-debug
cmake --build build/agent-debug --parallel 4
```

Agent ELF: `STM32F411CEU6/build/agent-debug/STM32F411CEU6.elf` from the
repository root. VS Code uses its own Debug build; rebuild it before flashing.
Build success is not board validation.

STM32F411CEU6, FreeRTOS V11.3.0, GCC ARM_CM4F, heap_4/16 KiB,
1000 Hz tick, NVIC group 4/four priority bits, syscall threshold 5,
EXTI0 priority 6; LED task priority 2, Heartbeat task priority 1.

The IRQ is software-pended: no GPIO/EXTI peripheral event is validated.
Read/modify/write is deliberately split using volatile saved values to make
the interleaving observable. This is not a disassembly claim about count++,
nor an estimate of natural race probability. Volatile is not synchronization.
The recorder briefly uses PRIMASK and restores its previous value; it
perturbs timing. This is not a precise latency or all-IRQ boundary test.
The critical section only covers IRQs masked by this port's threshold;
higher-urgency interrupts are not covered by this comparison.

The original baseline application guide describes baseline behavior, not
this branch's two-case behavior. Use this document for this experiment.
Keep this experiment on its own branch; it is not a change to the shared baseline.
