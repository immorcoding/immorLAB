# F411CEU6 bootloader 相关事实（调研）

> 来源：immorcoding/notes#15。只收集事实与出处，**不做分区或协议决策**。
> 这是 agent 整理的资料，不代表学习者已掌握。
> 标 **〔不确定〕** 的条目未在一手资料中直接找到，或只能推导。

## 资料与版本

| 简称 | 文档 | 版本 |
|---|---|---|
| RM0383 | [STM32F411xC/E Reference manual](https://www.st.com/resource/en/reference_manual/rm0383-stm32f411xce-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) | Rev 4（2025-05） |
| DS10314 | [STM32F411xC/xE Datasheet](https://www.st.com/resource/en/datasheet/stm32f411ce.pdf) | Rev 8（2024-01） |
| PM0214 | [Cortex-M4 Programming manual](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf) | Rev 10（2020-03） |
| AN2606 | [System memory boot mode](https://www.st.com/resource/en/application_note/an2606-introduction-to-system-memory-boot-mode-on-stm32-mcus-stmicroelectronics.pdf) | Rev 71 |
| AN3965 | [STM32F40x/41x IAP using USART](https://www.st.com/resource/en/application_note/an3965-stm32f40xstm32f41x-inapplication-programming-using-the-usart-stmicroelectronics.pdf) | Rev 1（2011，F40x + 旧 SPL，只作旁证） |
| HAL/CMSIS | 本仓库 `STM32F411CEU6/`，CubeMX 生成（`STM32Cube FW_F4 V1.28.3`，HAL V1.8.5） | 见 [.ioc](../../STM32F411CEU6/STM32F411CEU6.ioc) |
| CubeF4 IAP 示例 | [STM32CubeF4 @60c8abe](https://github.com/STMicroelectronics/STM32CubeF4/tree/60c8abe0ae88a23bbd1c925358330031e36dd20f/Projects/STM32446E_EVAL/Applications/IAP) | F446 EVAL 示例，只作旁证 |
| ST VSCode 扩展 | 本机安装的 `stmicroelectronics.stm32cube-ide-debug-stlink-gdbserver-1.5.1`（`package.json` 和 `lib/adapter/STLinkDebugTargetAdapter.js`）；ST 社区文章 [STM32CubeIDE for VS Code debug configuration](https://community.st.com/t5/stm32-mcus/stm32cubeide-for-vs-code-debug-configuration-launch-json/ta-p/866870) | 1.5.1 |

dev.st.com 上的扩展文档（[debug.html](https://dev.st.com/stm32cube-docs/stm32cubeide-vscode/1.0.1/en/docs/markup/development/debug.html)）调研时访问超时，未读到。

---

## 1. Flash 扇区布局与擦写约束

**扇区布局**（RM0383 §3.3 Table 4，p.44）。F411CE 有 512 KB Flash，单 bank：

| 扇区 | 地址 | 大小 |
|---|---|---|
| 0 | 0x0800 0000 – 0x0800 3FFF | 16 KB |
| 1 | 0x0800 4000 – 0x0800 7FFF | 16 KB |
| 2 | 0x0800 8000 – 0x0800 BFFF | 16 KB |
| 3 | 0x0800 C000 – 0x0800 FFFF | 16 KB |
| 4 | 0x0801 0000 – 0x0801 FFFF | 64 KB |
| 5 | 0x0802 0000 – 0x0803 FFFF | 128 KB |
| 6 | 0x0804 0000 – 0x0805 FFFF | 128 KB |
| 7 | 0x0806 0000 – 0x0807 FFFF | 128 KB |
| System memory | 0x1FFF 0000 – 0x1FFF 77FF | 30 KB |
| OTP | 0x1FFF 7800 – 0x1FFF 7A0F | 528 B |
| Option bytes | 0x1FFF C000 – 0x1FFF C00F | 16 B |

PDF 抽出的文字表格有错位；上表按 §3.3 正文“4 个 16 KB + 1 个 64 KB + 3 个 128 KB”核对过。

**擦除粒度**：只能按扇区擦，或整片擦（mass erase）。mass erase 不擦 OTP 和选项字节。（RM0383 §3.5.2–3.5.3，p.49）

**写入规则**（RM0383 §3.5.4，p.50）：
- 不擦除也可以把位从 1 写成 0；要写成 1 必须先擦除。
- 一次写入不能跨 128 位（16 字节）行边界，否则置 `PGAERR`。
- 写入宽度必须和 `PSIZE` 一致，否则置 `PGPERR`。
- 每次编程或擦除时 HCLK 必须 ≥ 1 MHz。编程或擦除过程中复位，Flash 内容不保证正确。（§3.5，p.48）
- 解锁 `FLASH_CR` 要依次写入 KEY1 = 0x45670123、KEY2 = 0xCDEF89AB。顺序错误会产生总线错误，并锁住 `FLASH_CR` 直到复位。（§3.5.1，p.48）

**写入宽度与电压**（RM0383 Table 6，p.49；DS10314 Table 45，p.93）：

| 电压 | 最大并行宽度 / PSIZE | HAL 宏（`stm32f4xx_hal_flash_ex.h` [L153-156](../../STM32F411CEU6/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal_flash_ex.h#L153-L156)） |
|---|---|---|
| 2.7–3.6 V，外接 VPP（8–9 V） | x64 / `11` | `FLASH_VOLTAGE_RANGE_4` |
| 2.7–3.6 V | x32 / `10` | `FLASH_VOLTAGE_RANGE_3` |
| 2.1–2.7 V | x16 / `01` | `FLASH_VOLTAGE_RANGE_2` |
| 1.7–2.1 V | x8 / `00` | `FLASH_VOLTAGE_RANGE_1` |

如果 PSIZE 和电压不匹配，结果不可预测：读回正确，也可能保持不住。（RM0383 Table 6 下方 Note）

**擦写时间**（DS10314 Table 45，p.93，典型值 / 最大值）：

| 操作 | x8 | x16 | x32 |
|---|---|---|---|
| 16 KB 扇区擦除 | 400 / 800 ms | 300 / 600 ms | 250 / 500 ms |
| 64 KB 扇区擦除 | 1200 / 2400 ms | 700 / 1400 ms | 550 / 1100 ms |
| 128 KB 扇区擦除 | 2 / 4 s | 1.3 / 2.6 s | 1 / 2 s |
| 整片擦除 | 8 / 16 s | 5.5 / 11 s | 4 / 8 s |
| 字编程 | 16 / 100 µs | 同左 | 同左 |

寿命：10 kcycles（DS10314 Table 47，p.94）。

**擦写期间能否从 Flash 取指**：不能。RM0383 §3.5（p.48）说：Flash 正在写或擦时去读它，会让总线停住（stall），所以这期间无法从 Flash 取指令或数据。F411 只有一个 bank，没有边读边写（RWW）。
- 推论 **〔不确定，未实测〕**：bootloader 在 Flash 中运行时擦除 APP 扇区，CPU 会停住最长约 2 s（128 KB，x32）。这期间中断也无法从 Flash 取向量。
- 擦除后要处理缓存：如果被擦的数据在 I/D cache 里，要用 `FLASH_CR` 的 `DCRST/ICRST` 清缓存，并且只能在缓存关闭时清。（§3.5.5，p.51）

**选项字节与保护对 bootloader 的影响**（RM0383 §3.6，p.51–55）：
- `nWRPi`（0x1FFF C008）：可以给扇区 0–7 中的任意扇区单独设写保护（0 = 保护）。`SPRMOD = 1` 时这些位改作 PCROP。
- RDP Level 0（0xAA）：不保护。
- RDP Level 1（默认，即除 0xAA 和 0xCC 以外的任何值）：
  - 连着调试器，或从 RAM / System memory 启动时，不能读、写、擦 Flash，读会产生总线错误。
  - 从 Flash 启动时，用户代码可以正常读、写、擦。也就是说**自写 bootloader 仍能擦写 APP**。
  - 从 Level 1 降回 Level 0 会触发整片擦除，写保护位不变。
- RDP Level 2（0xCC）：
  - 禁止从 RAM 和 System memory 启动，**ROM bootloader 不能用**。
  - 永久关闭 JTAG/SWD。
  - 选项字节以后不能再改，且**不可逆**。
  - 从 Flash 启动的用户代码仍能擦写 Flash。
- 修改选项字节的流程：先解锁 `OPTKEY1 = 0x08192A3B`、`OPTKEY2 = 0x4C5D6E7F`，再写 `FLASH_OPTCR` 并置 `OPTSTRT`。（§3.6.2）

## 2. bootloader 跳到 APP 前后的事项

ST 没有一份文档专门讲“自写 bootloader 跳到 APP”的清单。下面每一项都标出依据。

| 事项 | 依据 |
|---|---|
| **MSP**：上电复位时，CPU 从 0x0000 0000 读初始 MSP，从 0x0000 0004 读复位向量（bit0 必须为 1，表示 Thumb）。软件跳转不会自动做这件事，要自己从 APP 向量表第 0 字读值设 MSP，再跳到第 1 字。 | PM0214 §2.1.3（p.19），§2.3.4（p.40） |
| ST 示例的做法：先检查 APP 首字是否是 RAM 地址，再 `__set_MSP(*(uint32_t*)APP)`，然后调用 `*(APP+4)`。这个示例跳转前**没有** DeInit，也没有关中断。 | [CubeF4 IAP_Main main.c L70-79](https://github.com/STMicroelectronics/STM32CubeF4/blob/60c8abe0ae88a23bbd1c925358330031e36dd20f/Projects/STM32446E_EVAL/Applications/IAP/IAP_Main/Src/main.c#L70-L79) |
| **VTOR**：复位值是 0。地址范围 0x80–0x3FFFFF80。对齐要求是“向量个数向上取 2 的幂”，最少 128 字。 | PM0214 §2.3.4（p.40），§4.4.4（p.227） |
| F411 HAL 模板注释要求 VTOR 是 0x200 的倍数。推导〔不确定〕：F411 约有 100 个向量，2 的幂取 128 字，即 0x200，与注释一致。 | [system_stm32f4xx.c L101-109](../../STM32F411CEU6/Core/Src/system_stm32f4xx.c#L101-L109) |
| 本工程 `USER_VECT_TAB_ADDRESS` 默认被注释掉，所以 `SystemInit()` **不写 VTOR**。推论：APP 会沿用跳转前 VTOR 的值。所以 APP 要自己设 VTOR，或者由 bootloader 在跳转前设好。 | [system_stm32f4xx.c L94, L179-181](../../STM32F411CEU6/Core/Src/system_stm32f4xx.c#L179-L181) |
| ST 对 APP 的两条要求：链接地址改到 APP 起始地址；向量表重定位（改 `VECT_TAB_OFFSET`）。 | AN3965 §6（p.11），CubeF4 [IAP_binary_template readme](https://github.com/STMicroelectronics/STM32CubeF4/blob/60c8abe0ae88a23bbd1c925358330031e36dd20f/Projects/STM32446E_EVAL/Applications/IAP/IAP_binary_template/readme.txt) |
| **改向量表后的屏障**：改了向量表再使能异常时，中间要加 DMB。〔不确定〕“写 VTOR 后加 DSB/ISB”是常见做法，但 PM0214 里没找到明确要求。 | PM0214 §2.2.4 附近（p.30） |
| **关中断、清 pending**：`NVIC_ICERx` 关闭中断，`NVIC_ICPRx` 清 pending。SysTick 和 PendSV 的 pending 在 `ICSR` 的 `PENDSTCLR/PENDSVCLR` 位。 | PM0214 §4.3.3（p.211），§4.3.5（p.213），§4.4.3（p.225） |
| ST 对“从用户代码跳到 **ROM** bootloader”给的清单：关闭所有外设时钟、关闭用过的 PLL、关中断、清 pending 中断。〔不确定〕这份清单写给 ROM BL，用于自写 BL 到 APP 只是类比。 | AN2606 §4.1（p.37） |
| **SysTick**：`STK_CTRL` 的 `ENABLE`（bit0）、`TICKINT`（bit1）清零后，SysTick 停止计数，也不再产生中断。 | PM0214 §4.5.1（p.247） |
| `HAL_SuspendTick()` 只关 SysTick 中断。 | [stm32f4xx_hal.c L406-416](../../STM32F411CEU6/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c#L406-L416) |
| **`HAL_DeInit()` 做什么**：对 APB1/APB2/AHB1/AHB2/AHB3 外设强制复位再释放，然后调 `HAL_MspDeInit()`。它**不动时钟树，也不动 SysTick**。 | [stm32f4xx_hal.c L190-213](../../STM32F411CEU6/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c#L190-L213) |
| **`HAL_RCC_DeInit()` 做什么**：切回 HSI，关 HSE/PLL，分频设为 1，关 CSS/MCO，关 RCC 中断。它不改外设时钟使能和 LSI/LSE/RTC。注意：它最后会调 `HAL_InitTick()`，**SysTick 会被重新打开**。 | [stm32f4xx_hal_rcc_ex.c L3174-3186, L3346-3349](../../STM32F411CEU6/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c#L3346-L3349) |
| 推论：调用顺序会影响最终状态。先 `HAL_RCC_DeInit()` 再关 SysTick，SysTick 才会保持关闭。〔推导，未实测〕 | 同上 |
| APP 端：启动文件先 `ldr sp, =_estack` 再 `bl SystemInit`。所以 APP 的 Reset_Handler 会再设一次 SP。 | [startup_stm32f411xe.s L61-64](../../STM32F411CEU6/startup_stm32f411xe.s#L61-L64) |

## 3. ST 内置 ROM bootloader（System memory）

- **进入方式**：BOOT0 = 1、BOOT1 = 0（AN2606 Pattern 1，§38.1 p.190；RM0383 §2.4 Table 2 p.41）。
  - 复位后第 4 个 SYSCLK 上升沿锁存 BOOT 引脚。从 Standby 唤醒时也会重新采样。
  - BOOT1 和 PB2 复用（DS10314 引脚表）。
- 也可以从用户代码直接跳到 System memory（AN2606 §4.1 p.37）。跳转前要关外设时钟、关 PLL、关中断、清 pending。
- 退出方式：撤掉进入条件后复位，或用 Go 命令。Go 跳到用户代码时，**bootloader 用过的外设寄存器不恢复默认值**。比如 IWDG 预分频被设成最大，APP 要自己重新配置。（§4.1 p.37）
- **接口**（AN2606 Table 84 p.190–192；RM0383 §2.4 p.42）：
  - USART1（PA9/PA10）、USART2（PD5/PD6）：8 位、偶校验、1 停止位，自动识别波特率（主机先发 0x7F）。
  - I2C1/2/3：最高 400 kHz，7 位地址 0b0111001x。
  - SPI1/2/3：从机，最高 8 MHz，CPOL/CPHA = 0。
  - USB OTG FS DFU（PA11/PA12）：必须有 HSE，频率是 1 MHz 的整数倍，范围 4–26 MHz。ST 建议用低频晶振，例如 8 MHz 比 25 MHz 好。
- **资源占用**（Table 84）：
  - 系统时钟：用 HSI + PLL 跑 60 MHz。
  - RAM：占用 0x2000 0000 起的 12 KB。
  - System memory：0x1FFF 0000 起的 29 KB 是 bootloader 代码。
  - 电压范围默认 1.62–2.1 V，所以只能按字节写 Flash。可以用 bootloader 命令修改。
- **版本**（Table 85 p.194；§4.2 Table 3 p.42）：V13.0，ID 0xD0，版本字节地址 0x1FFF76DE。已知限制：Go 命令后 `AHB1ENR` 被清为 0。
- **RDP 影响**：Level 2 时不能从 System memory 启动。从用户代码跳进去时，只有 Get、GetID、GetVersion 命令可用。（AN2606 §4.1 p.35）
- 各接口的协议细节在 AN3155（USART）、AN3156（DFU）、AN4221（I2C）、AN4286（SPI）里。**〔本次未阅读〕**

## 4. VTOR 偏移和链接脚本的 Flash 起始地址在哪里设

本工程是 CubeMX 生成的 CMake 工程。

- **VTOR**：在 [`Core/Src/system_stm32f4xx.c` L94-111](../../STM32F411CEU6/Core/Src/system_stm32f4xx.c#L94-L111)。
  - 取消注释 `#define USER_VECT_TAB_ADDRESS` 后，`SystemInit()` 会执行 `SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET;`（[L179-181](../../STM32F411CEU6/Core/Src/system_stm32f4xx.c#L179-L181)）。
  - `VECT_TAB_BASE_ADDRESS` 默认是 `FLASH_BASE`；定义 `VECT_TAB_SRAM` 时为 `SRAM_BASE`。
  - `VECT_TAB_OFFSET` 外面有 `#if !defined(...)` 保护，所以可以用编译选项 `-DVECT_TAB_OFFSET=...` 覆盖。`USER_VECT_TAB_ADDRESS` 本身也可以用 `-D` 定义。这一点是看代码得出的。
  - 〔不确定〕CubeMX 重新生成代码时是否会覆盖 `system_stm32f4xx.c` 里的手改内容，没查到官方说明。这个文件里没有 `USER CODE` 保护段。
  - 〔不确定〕CubeMX 界面里有没有直接设置 VTOR 偏移的选项，本次没有核实。
- **链接地址**：在 [`STM32F411xx_FLASH.ld` L56-60](../../STM32F411CEU6/STM32F411xx_FLASH.ld#L56-L60) 的 `MEMORY { FLASH (rx) : ORIGIN = 0x8000000, LENGTH = 512K }`。APP 要同时改 ORIGIN 和 LENGTH。
  - 这个链接脚本由 [`cmake/gcc-arm-none-eabi.cmake` L43](../../STM32F411CEU6/cmake/gcc-arm-none-eabi.cmake#L43) 用 `-T` 传给链接器。
  - 文件里同样没有 `USER CODE` 段。
- 改完后可以检查两处〔建议，非 ST 原文〕：
  - APP 的 `.isr_vector` 是否落在新的 ORIGIN（看 `.map` 文件）。
  - `VECT_TAB_OFFSET` 是否等于 ORIGIN − 0x0800 0000。

## 5. VSCode STM32Cube 扩展 + ST-Link GDB Server：`launch.json` 加载两份镜像

依据是本机扩展 `stm32cube-ide-debug-stlink-gdbserver` **1.5.1** 的 schema（`package.json`）和适配器代码（`lib/adapter/STLinkDebugTargetAdapter.js`）。本工程当前配置见 [STM32F411CEU6/.vscode/launch.json](../../STM32F411CEU6/.vscode/launch.json)。

- `imagesAndSymbols` 是一个数组。每一项可以有 4 个字段：`imageFileName`、`imageOffset`、`symbolFileName`、`symbolOffset`。schema 的说明是 “List of image and symbol files to load at an optional offset”。
- **字段名要注意**：schema 里是 `symbolFileName`，没有 s。ST 社区文章写成 `symbolsFileName`，和 1.5.1 的 schema 不一致，以 schema 为准。社区文章的评论区还有用户反映 `device`、`interface` 等字段在自己的版本里不被接受。
- 文件名可以写成普通路径字符串，也可以写成 `${command:st-stm32-ide-debug-launch.get-projects-binary-from-context1..5}`。
  - 用 context 命令时：只找到一个 `.elf` 就直接用它；找到多个时，会弹出 “Select binary file for context N” 让人选。（debug-core 1.5.1 `lib/extension.js`）
- **适配器实际执行的顺序**（从代码读出）：
  1. `-file-exec-and-symbols <数组最后一项的 imageFileName，没有就用 symbolFileName>`。最后一项成为 GDB 的主程序。
  2. 对**每一项**执行 `add-symbol-file <symbolFileName，没有就用 imageFileName> <symbolOffset>`。这是 ST-Link 适配器重写的行为：只写 `imageFileName` 的项也会加载符号。
  3. 连接目标，复位策略由 `serverReset` 决定。
  4. 设置了 `runEntry` 时执行 `tbreak <runEntry>`。
  5. 按数组顺序对每个有 `imageFileName` 的项执行 `load <file> <imageOffset>`。
- 所以 boot + app 可以这样写：两项都写 `imageFileName`，也可以分别写 `symbolFileName`。数组最后一项会成为主 ELF。
- 〔不确定，未实测〕
  - 两个 ELF 都有 `main` 时，`runEntry: "main"` 产生的 `tbreak main` 可能停在两个位置中的任意一个，或者两个位置都下断点。这取决于 GDB 的行为。
  - 两个 ELF 自带链接地址，一般不需要填 offset。
  - 只附加调试、不烧录时：社区文章建议只写 symbol、`serverReset` 设为 `"None"`、删掉 `runEntry`。schema 里 `None` 的说明是 “Attach to the running target without reset”。
