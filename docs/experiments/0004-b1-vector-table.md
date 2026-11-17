# B1 上电后发生了什么：向量表到 main

> 本文由 agent 根据 2026-11-17 的教学对话整理。标“学习者原话”的部分是学习者自己说的，其余为 agent 整理。按 spec，agent 整理的内容不算掌握证据。

## 身份与状态

- 学习票：[immorcoding/notes#21](https://github.com/immorcoding/notes/issues/21)
- 分支：`lab/b1-vector-table`，从 tag `f411-baseline-v1`（`88e7b4e270ed55631d89fdb69605774d71f93a1c`）开出
- 板子：STM32F411CEU6，BOOT0 = 0
- 调试器：J-Link，VS Code + STM32Cube 调试扩展 1.5.1（`jlinkgdbtarget`）
- 代码改动：没有改固件。只改了 `STM32F411CEU6/.vscode/launch.json`：新增 J-Link 配置；J-Link 和 ST-Link 两项的 `runEntry` 都从 `main` 改为 `Reset_Handler`。

## 运行前预测

如实记录哪些预测是在看结果之前给出的：

| 预测 | 什么时候给出 | 学习者原话 |
|---|---|---|
| 向量值为 0x0800 01C4 的 Thumb 函数，表里存 0x0800 01C5；存 0x0800 0299 时，从 0x0800 0298 取指 | 上板之前（课程自查题） | “0x0800 01C5，0x0800 0299 cpu 从 0x0800 0298 取第一条” |
| 执行 `ldr sp, =_estack` 后，`$sp` 不变 | 单步之前 | “不会变” |
| 上一条的理由 | 和结果同时给出 | “硬件本身已经指过sp到ram区的栈顶了，这里是再指一次同样的” |

向量表两个字的值**没有**事先预测：学习者先在 `.map` 中查到了 `_estack` 和 `Reset_Handler`，再用 GDB 读取。

## 步骤

1. 编译基线，在 `build/Debug/STM32F411CEU6.map` 中查 `_estack` 和 `Reset_Handler`。
2. 把**正在使用的** J-Link 配置的 `runEntry` 改为 `Reset_Handler`，停止调试后重新按 F5 启动。
3. 停在 `startup_stm32f411xe.s:61`（`ldr sp, =_estack`）。在调试控制台逐条输入 `>x/2xw 0x08000000`、`>x/2xw 0x00000000`、`>p/x $pc`、`>p/x $msp`。
4. 按 F10 单步一次，再输入 `>p/x $sp`。

## 结果

| 观察 | 实际值 | 对照 |
|---|---|---|
| `.map`：`_estack` | `0x20020000` | `ORIGIN(RAM) + LENGTH(RAM)` = 0x2000 0000 + 128K |
| `.map`：`Reset_Handler` | `0x08001674`（偶数） | 函数第一条指令的真实地址 |
| `x/2xw 0x08000000` | `0x20020000  0x08001675` | 第 0 字 = `_estack`；第 1 字 = `Reset_Handler` + 1 |
| `x/2xw 0x00000000` | `0x20020000  0x08001675` | 与 0x0800 0000 相同：BOOT0 = 0 时的别名 |
| 停在 Reset_Handler 时 `$pc` | `0x08001674` | 向量值去掉 bit0 |
| 停在 Reset_Handler 时 `$msp` | `0x20020000` | 硬件从第 0 字装入 |
| 停在 Reset_Handler 时 `xpsr` | `0x01000000` | bit24 = EPSR.T = 1，来自向量值的 bit0 |
| 停在 Reset_Handler 时 `lr` | `0xFFFFFFFF` | PM0214 §2.1.3 规定的复位值 |
| 单步后 `$pc` | `0x08001678`（+4） | `ldr sp, =_estack` 是 32 位 Thumb 指令 |
| 单步后 `$sp` | `0x20020000`（不变） | 与预测一致 |
| 单步时 J-Link 日志 | `Read 4 bytes @ 0x080016AC (Data = 0x20020000)` | `=_estack` 的值放在文字池（literal pool）里，指令以 PC 相对方式读取 |

## 差异与解释

- **第一次读到 `$msp = 0x2001FFF8`，比 `_estack` 少 8 字节。** 原因：当时在用的 J-Link 配置 `runEntry` 还是 `main`（改的是 ST-Link 那一项），CPU 已经跑进了 `main`。至于少 8 字节，**推测**是 `main` 开头的 `push {r7, lr}`，**未验证**。
- **调试控制台报 `could not evaluate expression`。** ST 扩展把不带 `>` 的输入当成 C 表达式求值（见扩展源码 `JLinkDebugTargetAdapter.js` 的 `evaluateRequest`）。GDB 命令必须加 `>` 前缀；`-exec` 是 cppdbg 扩展的写法，这里无效。agent 一开始给错了这条提示。

## 学习者在对话中澄清的概念

agent 记录，尚未通过独立题目检验：

- F411 没有 MMU，0x0 和 0x0800 0000 都是物理地址，属于别名（aliasing），不是虚拟地址。
- 指令长度（ARM 32 位 / Thumb 16 或 32 位）和地址位数是两回事。指令按 2 或 4 字节对齐，所以真实地址的 bit0 恒为 0，才能拿来当 Thumb 标志。
- 链接脚本里的 VMA 和 LMA：`.data` 运行在 RAM（`_sdata`），初值存放在 Flash（`_sidata = LOADADDR(.data)`）。

## 局限

- 只验证了 BOOT0 = 0（从主 Flash 启动）的情况，没有试 System memory 或 SRAM 启动。
- 只单步到 `ldr sp, =_estack`。`SystemInit`、`.data` 拷贝、`.bss` 清零、`__libc_init_array` 都没有逐步观察。
- 是通过调试器复位进入 `Reset_Handler` 的，不是真正的上电复位。
- 0x2001FFF8 的成因是推测。
