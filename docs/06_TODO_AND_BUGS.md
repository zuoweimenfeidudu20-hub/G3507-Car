# 06 - 已知缺陷诊断与开发路线图 (Bug Tracker & Roadmap)

> **[IMPORTANT] 本文档记录了当前代码中存在的阻碍实车运行的隐患及后续迭代目标。**

---

## 1. 致命缺陷清单 (Critical Bugs)

### 调试连接约定

烧录脚本默认使用 TI XDS110。VS Code 提供 `attach (no flash)` 和显式的 `build, flash and debug` 两个 F5 配置；前者只连接当前固件，不构建或下载，后者构建并烧录。此前 OpenOCD 已识别 XDS110（USB VID/PID `0451:bef3`，固件 `3.0.0.43`）；`probe.ps1 -Action Check` 成功读取目标 SWD DPIDR `0x6ba02477` 并识别 MSPM0G3507 Cortex-M0+。`probe.ps1 -Action Flash` 此前已在实机完成写入、校验和复位。2026-10-30 截图显示 VS Code 已进入图形调试会话；同日离线排查时 XDS110 连接检查返回 `XDS110: failed to connect`，所以本轮没有对芯片执行调试或烧录操作。

### ✅ BUG-00: 按键 GPIOB 中断与轮询并存（已修复，待实机验证）

- 按键由 `key_timer` 定时扫描并消抖，原 `init.syscfg` 却同时给四个 GPIOB 按键开启双边沿中断；当前扫描周期改为 5 ms，消抖阈值为 6 次（30 ms）。
- GPIOA 与 GPIOB 共用 `GROUP1_IRQHandler`；现有入口只清理 GPIOA 编码器中断，按键边沿产生的 GPIOB 中断标志无法清除，可能导致中断反复进入、其他按键和主循环失去响应。
- 已关闭四个按键的 GPIOB 边沿中断，保留内部上拉和定时轮询。需在实机上验证长时间运行后各按键仍可切换模式。

### ⚠️ BUG-00A: 串口遥测可能阻塞主循环（已加等待上限，待实机验证）

- 主循环内的 `motor_rlencoder_cnts()` 调用 `UART_SendString()`；原实现逐字节调用会无限等待的 `DL_UART_Main_transmitDataBlocking()`。当前循迹仍每 10 ms 控制一次，但遥测改为每 5 个样本发送一次（约 20 Hz），避免长报文在 115200 bps 下占满带宽并出现截图中的断裂/粘连行。
- 若 UART 发送器停止出队，主循环会停在串口发送，无法读取新的按键模式；现改为每字节有限次尝试，超限即放弃本次报文，并累加 `uart_tx_timeout_count` 供调试器查看。
- 串口停止与按键失效同时发生，但尚未确认 UART 是否为实际停机点；还需在失效时读取 CPU 状态和计数器。
- 按键 4 的停车目标现在直接由消抖完成的按键中断下发，不再依赖主循环先从 UART 调用返回；若中断本身停摆，此保护也无法生效。

### ⚠️ BUG-00B: 循迹中突然回到按键 4 待机（已确认芯片复位，供电/NRST 根因待查）

以下 2026-10-30 历史日志使用旧引脚分配，K4 当时为 `PB13`；当前引脚分配已改为 `PB7`。

- 2026-10-30 串口截图显示 `key1_putong/state1_c_lockstraight1` 后出现 `key4_idle/state=inactive`。循迹子状态机不会写入按键模式；在当前正常代码路径中，运行时切到 K4 来自 PB13 连续约 30 ms 被读为低电平，另一种可能是 MCU 重启后 `mode` 回到上电默认的 `key4_idle`。现有截图无法区分这两种原因。
- 现加 `event=boot,reset=...,reason=...` 启动标记及 `event=key4_confirmed,count=...` 消抖确认标记。若现象前后出现新的 `event=boot`，按 `reason` 排查供电跌落、复位脚或其他重启原因；若出现 `event=key4_confirmed` 而没有新启动标记，检查 PB13（K4）实际电平、接线、内部上拉及电机工作时的干扰。`reason=bor_supply` 表示芯片检测到供电欠压复位。暂不盲目延长 K4 消抖，以免拖慢急停响应。
- 主循环也在循迹计算后复查按键模式；若 K4 在本轮模式快照之后被确认，立即再次将双轮目标清零。
- 后续实机日志已在 `key1_putong/state1_c_lockstraight1` 后捕获多次 `event=boot,reset=0x04,reason=bor_supply`，没有 `event=key4_confirmed`；一次 `event=boot,reset=0x15,reason=cpu_lockup` 也夹在反复欠压复位中。因此本次回到 K4 是芯片重启后回到默认模式，并非寻迹状态机或 K4 误触发。`0x04` 是 MCU 的 BOR 供电欠压复位，`0x15` 是 CPU lockup 复位；后者是否由供电扰动连带造成，尚不能单凭日志确定。
- 先用示波器在 MCU 供电脚相对 MCU 地脚测 3.3 V，尤其观察电机启动、左右轮 PWM 变化和右岔道附近的瞬时跌落；再在电机不驱动时复现，区分电机负载与控制板/传感器供电问题。检查电源/稳压器容量、线束和地线接触、MCU 近端去耦、电机驱动供电，以及曾发生过短路的灰度板/排线。若供电稳定后仍出现 `cpu_lockup`，再单独排查软件或中断。
- 再次实机日志先出现 `event=boot,reset=0x04,reason=bor_supply`，随后出现 `event=boot,reset=0x0c,reason=nrst_pin`。本机 SDK 定义 `0x0c` 为 NRST 低脉冲（短于 1 秒）引发的 BOOTRST；与 K4/PB13 无关。`gray=00000000,pattern_lost` 只表示当时 8 路灰度输入全低，不能单独证明灰度板失电。
- 下一步同时用示波器测 MCU 的 VDD–VSS 和 NRST–VSS，并以 NRST 下降沿触发：VDD 先跌时查电源、地线、电机负载；VDD 稳定而 NRST 单独出现低脉冲时查复位按钮、XDS110/调试接线、复位线上拉与抗干扰电容。两路同时变化时依波形先后判断，不能凭两个复位码断言有两个独立硬件故障。[TI MSPM0 G 系列硬件指南](https://www.ti.com/lit/an/slaae76e/slaae76e.pdf)指出 NRST 没有内部上拉，需外部电路拉高。

### ✅ BUG-01: 未完成的 ECHO 中断与编码器共用 GPIOA（已停用超声波配置）
- **处理记录**：`init.syscfg` 已删除超声波 GPIO 配置（原 `ECHO PA1`、`TRIG PB14`）和 `TIMG6` 周期定时器配置。`pid_motor.c` 的 `GROUP1_IRQHandler` 不再调用 `HCSR04_HandleEchoInterrupt()`，只处理当前配置的编码器引脚。
- `hcsr04.c` 仍在构建文件列表中，但只有 SysConfig 同时生成 ECHO、TRIG 和定时器宏时才编译测距实现；当前固件不包含超声波测距函数或中断处理函数。超声波避障当前不可用。
- **待验证**：上板观察编码器计数和按键切换是否仍出现中断卡住；此前故障尚未确认仅由 ECHO 引起。

---

### ✅ BUG-02: `init.c` 主循环尚未接入循迹主控（已修复，待实机验证）
- **问题文件**：[init.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.c)
- **修复记录**：主循环已接入 `key_getmode()` → `gray_sample()` → `trackcontroll()`；按键 1 执行 `track_run_normal()` 进行灰度循迹，按键 4 将双轮目标置零。启动目标为 0。`TIMG8` 每 10 ms 发布一次包含 8 路读数、`pattern` 和 `error` 的采样。
- **待验证**：上板确认按键电平、灰度黑线电平、左右转向符号和 PID 参数。

---

### ⚠️ BUG-03: 转向环参数需要实车整定
- **问题文件**：[init/grayscale_sensor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\grayscale_sensor.c)
- **现状**：`control.kp = 10`、`output_limit = 10`；按键 1 普通循迹误差经过 `line_pid()`，不会把 ±3500 的灰度权重直接当成 PWM 差值。
- **待验证**：目前误差绝对值达到约 100 时输出即被限制为 ±10，转向可能不足；根据实车传感器和电机响应整定增益与限幅。

---

## 2. 后续功能迭代路线图 (Roadmap)

1. [ ] **第一阶段：运动闭环调优**
   - 调试编码器脉冲回传，验证 `pid_motor` 速度闭环稳态性与超调量；
   - 验证串口遥测输出波形正常。
2. [ ] **第二阶段：基础黑线寻迹**
   - 实车验证已接入的 `trackcontroll()` 主循环路径；
   - 整定转向 PID 参数（先调 P 消除静差，再调 D 抑制过弯振荡）；
   - 测试出界丢线保护（`state_stop`）。
3. [ ] **第三阶段：复杂赛道适应**
   - 调优 Y/T 型分支路况识别状态机（`pattern_rightbranch`, `pattern_leftbranch`）；
   - 验证台阶/特殊路段跳变判断（`center_bianhua`）。
4. [ ] **第四阶段：外设融合**
   - 如需恢复 HC-SR04，重新设计引脚、定时器与共享中断分发，完成测距状态机后再接入前向防撞刹车；
   - 读取 JY901P 航向角用于直角转弯（90° 闭环打角）。

---

## 3. 调试器字符集与硬件断点资源（配置已调整，待实机验证）

- **现象**：Cortex-Debug 启动时，Arm GNU Toolchain 14.2.Rel1 所带 GDB 在执行 `-break-insert -t --function main` 时提示无法从 CP1252 转换到 UTF-32。
- **定位**：使用同一 GDB 和 `build/init.out` 离线复现；断点仍成功建立，目标程序也已在 `main` 停止。该 GDB 的 `set host-charset` 仅接受 `auto` 和 `CP1252`，无法通过设置 UTF-8 解决。
- **原处理与新问题**：原配置禁用了扩展自动生成的函数断点，但在启动、重启和复位后执行 `tbreak *main`。2026-10-30 的调试截图显示 4 个启用的人工源码断点；启动时再加 `main` 临时断点，可能需要 5 个硬件断点，而 MSPM0G3507 最多只有 4 个。GDB 报告 `Cannot insert hardware breakpoint`，随后 Cortex-Debug 报内部错误。
- **本轮处理**：两个 VS Code 配置都不再自动设置 `main` 临时断点，保留 `runToEntryPoint: ""`。新增 `request: attach` 配置用于只连接当前固件，不构建或下载。逐过程调试可能需要临时断点，建议最多同时启用 3 个人工断点。
- **界限**：截图停在 `key_getmode()` 的正常入口 `0x1e20`，而 `HardFault_Handler` 在 `0x1812`；单凭编辑器的“exception”字样不能认定 `key.c` 发生 HardFault。由于本轮 XDS110 未连通，修复只完成静态验证，尚待实机确认。

## 4. UART0 灰度与电机遥测（已接入，待实机验证）

- `init.syscfg` 原已配置 UART0（PA10 TX、PA11 RX，115200 bps），生成代码会在 `SYSCFG_DL_init()` 中使能该外设。
- `init.c` 每次获取灰度样本并更新状态后（最多约每 10 ms 一次）调用 `motor_rlencoder_cnts(&sample, current_mode)`，输出同一次采样的 8 路读数、`pattern` 名称、进入的状态名称、消抖后识别出的按键模式，以及左右电机 PID 下发的 PWM 值。主循环若被串口发送拖慢，后续灰度样本可能覆盖尚未处理的样本。
- 编译已通过；仍需接线并在串口终端以 115200、8N1 验证实机输出。
