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

### 🐛 BUG-02: `init.c` 主循环尚未接入循迹主控
- **问题文件**：[init.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.c)
- **现象分析**：
  - 主循环中的 `switch(current_mode)` 分支（`key1_putong`, `key2_teshu`, `key3_baidong`）目前全为空语句；
  - `track_controll()` 并没有被任何定时器或主循环周期调用，小车启动后虽然按键能切模式，但小车仅会保持 0 速度或直行，无法自动寻线。
- **推荐修复方案**：
  在 `key1_putong` 模式下周期调用 `track_controll()`，并配合 `delay_ms(10)` 或由 `TIMG8 (track_timer)` 调度。

---

### 🐛 BUG-03: 转向环 PID 参数未赋初值
- **问题文件**：[init/grayscale_sensor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\grayscale_sensor.c)
- **现象分析**：
  - `control` 结构的 `kp = 0, ki = 0, kd = 0`，导致 `line_pid()` 输出的 `correction` 始终为 0，即便调用了寻迹逻辑小车也不会产生差速打角。
- **推荐修复方案**：
  整定一套基础参数，例如先从 PD 控制开始：
  ```c
  track_pid control = {
      .kp = 20,
      .ki = 0,
      .kd = 5,
      .integral = 0,
      .previous_error = 0,
      .track_int_limit = 500,
      .output_limit = 15
  };
  ```

---

## 2. 后续功能迭代路线图 (Roadmap)

1. [ ] **第一阶段：运动闭环调优**
   - 调试编码器脉冲回传，验证 `pid_motor` 速度闭环稳态性与超调量；
   - 验证串口遥测输出波形正常。
2. [ ] **第二阶段：基础黑线寻迹**
   - 接入 `track_controll()` 到主循环；
   - 整定转向 PID 参数（先调 P 消除静差，再调 D 抑制过弯振荡）；
   - 测试出界丢线保护（`state_stop`）。
3. [ ] **第三阶段：复杂赛道适应**
   - 调优 Y/T 型分支路况识别状态机（`pattern_rightbranch`, `pattern_leftbranch`）；
   - 验证台阶/特殊路段跳变判断（`center_bianhua`）。
4. [ ] **第四阶段：外设融合**
   - 修复中断后使能 HC-SR04 超声波动态测距与前向防撞刹车；
   - 读取 JY901P 航向角用于直角转弯（90° 闭环打角）。
