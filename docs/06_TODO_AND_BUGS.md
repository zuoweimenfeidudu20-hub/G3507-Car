# 06 - 已知缺陷诊断与开发路线图 (Bug Tracker & Roadmap)

> **[IMPORTANT] 本文档记录了当前代码中存在的阻碍实车运行的隐患及后续迭代目标。**

---

## 1. 致命缺陷清单 (Critical Bugs)

### 🐛 BUG-01: GPIOA 共享中断冲突导致超声波测距瘫痪
- **问题文件**：[init/pid_motor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\pid_motor.c)、[init/hcsr04.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\hcsr04.c)
- **现象分析**：
  - MSPM0 的所有 GPIOA 引脚共用同一个中断向量：`GROUP1_IRQHandler`。
  - 右轮编码器（`PA15, PA16`）和左轮编码器（`PA17, PA22`）挂在 GPIOA；
  - 超声波测距的 `ECHO` 引脚同样挂在 GPIOA 的 `PA1`；
  - 目前 `pid_motor.c` 独占了 `GROUP1_IRQHandler` 的实现，仅处理了编码器引脚的挂起标志；
  - `hcsr04.c` 中实现的 `HCSR04_HandleEchoInterrupt()` 根本未被调用！导致如果调用 `HCSR04_Init()`，不但超声波收不到脉冲，还可能因未清除 `PA1` 的中断标志位而导致系统陷入死循环中断。
- **推荐修复方案**：
  在 `GROUP1_IRQHandler` 中增加对 `HCSR04_ECHO_PIN` 的判断：
  ```c
  #include "hcsr04_internal.h"
  
  void GROUP1_IRQHandler(void)
  {
      // 1. 优先处理超声波 ECHO 中断
      if (DL_GPIO_getEnabledInterruptStatus(GPIOA, HCSR04_ECHO_PIN)) {
          HCSR04_HandleEchoInterrupt();
      }
      
      // 2. 继续处理编码器引脚中断
      int right_pins = MOTOR_DIR1_CH_1_PIN | MOTOR_DIR1_CH_2_PIN;
      int left_pins = MOTOR_DIR2_CH_11_PIN | MOTOR_DIR2_CH_22_PIN;
      int pending = DL_GPIO_getEnabledInterruptStatus(GPIOA, right_pins | left_pins);
      if (pending != 0) {
          DL_GPIO_clearInterruptStatus(GPIOA, pending);
          // 状态查表逻辑...
      }
  }
  ```

---

### ✅ BUG-02: `init.c` 主循环尚未接入循迹主控（已修复，待实机验证）
- **问题文件**：[init.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.c)
- **修复记录**：主循环已接入 `key_getmode()` → `gray_sample()` → `trackcontroll()`；按键 1 进入普通循迹，按键 4 将双轮目标置零。启动目标也改为 0。`TIMG8` 每 40ms 发布一次采样。
- **待验证**：上板确认按键电平、灰度黑线电平、左右转向符号和 PID 参数。

---

### ⚠️ BUG-03: 转向环参数需要实车整定
- **问题文件**：[init/grayscale_sensor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\grayscale_sensor.c)
- **现状**：`control.kp = 10`、`output_limit = 5`，误差现已经过 `line_pid()`，不会再把 ±3500 的灰度权重直接当成 PWM 差值。
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
- `init.c` 现于每 5 次灰度采样（约 200ms）调用 `motor_encoder_condition()`，输出目标、实际 PWM、编码器计数与 PID 积分项。
- 编译已通过；仍需接线并在串口终端以 115200、8N1 验证实机输出。
