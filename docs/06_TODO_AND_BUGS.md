# 06 - 已知缺陷诊断与开发路线图 (Bug Tracker & Roadmap)

> **[IMPORTANT] 本文档记录了当前代码中存在的阻碍实车运行的隐患及后续迭代目标。**

---

## 1. 致命缺陷清单 (Critical Bugs)

### 调试连接约定

烧录脚本默认使用 TI XDS110，VS Code 的 F5 配置也通过 XDS110 构建、烧录和调试。OpenOCD 已识别 XDS110（USB VID/PID `0451:bef3`，固件 `3.0.0.43`）；`probe.ps1 -Action Check` 成功读取目标 SWD DPIDR `0x6ba02477` 并识别 MSPM0G3507 Cortex-M0+。`probe.ps1 -Action Flash` 已在实机完成写入、校验和复位。通过同一 OpenOCD 服务连接 GDB，已成功下载 `build/init.out`、读取 PC/SP、设置临时断点并停在 `main()`。VS Code 的 F5 图形界面尚未直接操作验证。初次连接曾短暂出现 `Error connecting DP: cannot read IDR`，随后多次重试成功。

### ✅ BUG-00: 按键 GPIOB 中断与轮询并存（已修复，待实机验证）

- 按键由 `key_timer` 定时扫描并消抖，原 `init.syscfg` 却同时给四个 GPIOB 按键开启双边沿中断；当前扫描周期改为 5 ms，消抖阈值为 6 次（30 ms）。
- GPIOA 与 GPIOB 共用 `GROUP1_IRQHandler`；现有入口只清理 GPIOA 编码器中断，按键边沿产生的 GPIOB 中断标志无法清除，可能导致中断反复进入、其他按键和主循环失去响应。
- 已关闭四个按键的 GPIOB 边沿中断，保留内部上拉和定时轮询。需在实机上验证长时间运行后各按键仍可切换模式。

### ⚠️ BUG-00A: 串口遥测可能阻塞主循环（已加等待上限，待实机验证）

- 主循环内的 `motor_encoder_condition()` 调用 `UART_SendString()`；原实现逐字节调用会无限等待的 `DL_UART_Main_transmitDataBlocking()`。用户当前调试版最多每次 10 ms 灰度采样都发送一次。
- 若 UART 发送器停止出队，主循环会停在串口发送，无法读取新的按键模式；现改为每字节有限次尝试，超限即放弃本次报文，并累加 `uart_tx_timeout_count` 供调试器查看。
- 串口停止与按键失效同时发生，但尚未确认 UART 是否为实际停机点；还需在失效时读取 CPU 状态和计数器。
- 按键 4 的停车目标现在直接由消抖完成的按键中断下发，不再依赖主循环先从 UART 调用返回；若中断本身停摆，此保护也无法生效。

### ⚠️ BUG-01: GPIOA 共享中断已接入 ECHO，测距流程仍待完成
- **问题文件**：[init/pid_motor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\pid_motor.c)、[init/hcsr04.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\hcsr04.c)
- **现象分析**：
  - MSPM0 的所有 GPIOA 引脚共用同一个中断向量：`GROUP1_IRQHandler`。
  - 右轮编码器（`PA15, PA16`）和左轮编码器（`PA17, PA22`）挂在 GPIOA；
  - 超声波测距的 `ECHO` 引脚同样挂在 GPIOA 的 `PA1`；
  - `pid_motor.c` 的 `GROUP1_IRQHandler` 现已调用 `HCSR04_HandleEchoInterrupt()`，先清除 ECHO 中断标志，再处理编码器引脚，避免共享 IRQ 被未清理的 ECHO 标志反复触发。
  - 测距流程尚未完成：`hcsr04.c` 的 `s_enabled` 未设为 1，周期定时中断也未安排下一次触发；此修复只保证共享中断能正常退出，不代表超声波测距已可用。

---

### ✅ BUG-02: `init.c` 主循环尚未接入循迹主控（已修复，待实机验证）
- **问题文件**：[init.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.c)
- **修复记录**：主循环已接入 `key_getmode()` → `gray_sample()` → `trackcontroll()`；当前按键 1 分支固定下发 `set_target(70, 0)`，普通循迹函数仍被注释；按键 4 将双轮目标置零。启动目标也改为 0。`TIMG8` 每 10 ms 发布一次采样。
- **待验证**：上板确认按键电平、灰度黑线电平、左右转向符号和 PID 参数。

---

### ⚠️ BUG-03: 转向环参数需要实车整定
- **问题文件**：[init/grayscale_sensor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\grayscale_sensor.c)
- **现状**：`control.kp = 20`、`output_limit = 5`；普通循迹函数当前未调用，恢复后误差将经过 `line_pid()`，不会把 ±3500 的灰度权重直接当成 PWM 差值。
- **待验证**：目前误差达到 50 时输出即被限制为 ±5，转向可能不足；根据实车传感器和电机响应整定增益与限幅。

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
   - 修复中断后使能 HC-SR04 超声波动态测距与前向防撞刹车；
   - 读取 JY901P 航向角用于直角转弯（90° 闭环打角）。

---

## 3. 调试器字符集警告（已处理）

- **现象**：Cortex-Debug 启动时，Arm GNU Toolchain 14.2.Rel1 所带 GDB 在执行 `-break-insert -t --function main` 时提示无法从 CP1252 转换到 UTF-32。
- **定位**：使用同一 GDB 和 `build/init.out` 离线复现；断点仍成功建立，目标程序也已在 `main` 停止。该 GDB 的 `set host-charset` 仅接受 `auto` 和 `CP1252`，无法通过设置 UTF-8 解决。
- **处理**：`.vscode/launch.json` 禁用扩展自动生成的函数断点，在启动、重启和复位后执行 `tbreak *main`。这仍是随符号地址变化的临时断点，不需要写死地址；离线验证无字符集警告。

## 4. UART0 编码器遥测（已接入，待实机验证）

- `init.syscfg` 原已配置 UART0（PA10 TX、PA11 RX，115200 bps），生成代码会在 `SYSCFG_DL_init()` 中使能该外设。
- 当前调试版 `init.c` 每次获取灰度样本（最多约每 10 ms 一次）都调用 `motor_encoder_condition()`，输出目标、实际 PWM、编码器计数与 PID 积分项。主循环若被串口发送拖慢，后续灰度样本可能覆盖尚未处理的样本。
- 编译已通过；仍需接线并在串口终端以 115200、8N1 验证实机输出。
