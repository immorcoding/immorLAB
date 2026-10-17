# immorLAB

Small STM32/FreeRTOS learning experiments. The shared F411 baseline is
identified by the Git tag `f411-baseline-v1`.

## Layout

- `STM32F411CEU6/`: CubeMX/CMake firmware project.
- `STM32F411CEU6/App/`: experiment tasks, software IRQ, and memory trace.
- `docs/experiments/`: observations and evidence, separate from run instructions.
- `docs/agents/`: issue tracker, triage labels, and domain documentation rules.

Keep one Git repository at this outer root. Open `STM32F411CEU6/` directly
in VS Code for STM32 extension building and ST-Link debugging; the outer
workspace's debugging path has not been validated.

## Run the baseline

In the inner firmware directory, with the ARM GNU toolchain, CMake, and
Ninja available:

```powershell
cmake --preset Debug
cmake --build --preset Debug
```

Output: `STM32F411CEU6/build/Debug/STM32F411CEU6.elf` from the repository root.
Use the inner project's ST-Link launch configuration. See
[the application guide](STM32F411CEU6/App/README.md) for the one-shot IRQ
scenario and debugger observations. The baseline LED toggles once per reset;
it is not a periodic blink program.

## Start an experiment

Run from the Git repository root; replace the sample branch name as needed:

```powershell
git switch -c codex/lost-update f411-baseline-v1
```

Each experiment starts from the fixed baseline tag and keeps its own code,
prediction, observations, limitations, and full commit SHA. Experiment
results stay on their branches. Shared fixes can be reviewed separately
for inclusion in `main`; experiments are not automatically merged back.
Keep published baseline tags fixed; create a new version for a later baseline.

## Evidence

- [LED bring-up](docs/experiments/0001-f411-led-baseline.md).
- [Task/IRQ observations and review](docs/experiments/0002-f411-tasks-and-irq.md).
- Board-tested firmware: `eeddee910e6c4d7ea945e6fce0987ebeb200d177`.
- [Completed prototype](https://github.com/immorcoding/notes/issues/6).
- [Ongoing learning spec](https://github.com/immorcoding/notes/issues/5).

The baseline is deliberately small: direct HAL and FreeRTOS calls, no MP3
product layers or dependency inversion. The IRQ is software-pended in NVIC;
GPIO/EXTI hardware triggering and precise timing are not validated here.
