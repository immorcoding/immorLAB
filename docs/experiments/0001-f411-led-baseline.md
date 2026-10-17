# F411 LED baseline

Date: 2026-10-17

## Scope and evidence

- Target: STM32F411CEU6; firmware project: `STM32F411CEU6/`.
- The learner confirmed DFU and ST-Link connection tests succeeded.
- VS Code showed a successful Debug build with exit code 0.
- After creating the inner project's ST-Link launch configuration, the
  learner confirmed that the LED blinked normally on the board.
- Current source toggles PC13 (`LED_Pin`) and calls `HAL_Delay(1000)`
  in the main loop. This specifies an approximately two-second full cycle;
  no measured timing accuracy is claimed.

This establishes a basic build, programming, and execution path. It does
not establish FreeRTOS operation or independent mastery of concurrency.

## Working directory convention

- Keep the Git repository and Matt skill configuration at `immorLAB/`.
- Open `immorLAB/STM32F411CEU6/` directly in VS Code for building,
  programming, and debugging with the current STM32 extension.
- Keep firmware debug configuration in `STM32F411CEU6/.vscode/launch.json`.
- Use the outer directory in Codex to manage the whole experiment repository.
- Do not initialize a second Git repository inside the firmware directory.

## Debugging observations

The installed STM32CubeIDE extensions are version 1.5.0. In the outer
workspace, setting `cmake.sourceDirectory` allowed building, but debug
launches first lacked device information and then failed to open the
inner project's `.settings/ide.store.json`.

Opening the inner project caused the extension metadata to be generated.
The extension's standard ST-Link launch configuration then worked according
to the learner's board test. The temporary outer `launch.json` was removed.
Debugging from the outer workspace or a multi-root workspace remains unverified.

## Limits and next step

- Current configuration selects HSE and PLL, declares a 25 MHz HSE, and
  targets a 100 MHz system clock. The physical crystal marking and clock
  accuracy have not been independently checked; blinking alone does not
  validate the complete clock configuration.
- The outer directory is the Git repository, with `origin` set to
  `https://github.com/immorcoding/immorLAB.git`. This record and the working
  LED firmware are captured together in the initial local baseline commit
  on `main`. Obtain its full SHA with `git log --reverse --format=%H`.
  No FreeRTOS experiment branch or remote push has been made at this point.
- J-Link testing is deferred until the adapter cable arrives.
- Next: integrate FreeRTOS V11.3.0, verify two tasks and a controlled ISR,
  and compare the learner's predictions with observed events. Review the
  SysTick/HAL time-base arrangement before enabling the scheduler.

Related planning:
- https://github.com/immorcoding/notes/issues/5
- https://github.com/immorcoding/notes/issues/8
- https://github.com/immorcoding/notes/issues/9
