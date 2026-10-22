# 02 - 软件分层架构与系统调用流 (Software Architecture & Flow)

---

## 1. 软件四层体系结构 (Four-Layer Architecture)

本项目采用模块化嵌入式前后台分层架构，清晰划分各层职责与调用边界：

```mermaid
graph TD
    subgraph Layer4["第 4 层：业务决策与模式调度层 (Application Layer)"]
        MAIN["init.c : main()"]
        KEY_APP["key.c : 模式状态分发"]
    end

    subgraph Layer3["第 3 层：运动控制与状态机算法层 (Control & Algorithm Layer)"]
        TRACK_FSM["grayscale_sensor.c : 寻迹路况分析与动作状态机"]
        LINE_PID["grayscale_sensor.c : line_pid (转向纠偏)"]
        MOTOR_PID["pid_motor.c : pid_motor (10ms 双轮速度闭环)"]
    end

    subgraph Layer2["第 2 层：设备与传感器驱动层 (Device Drivers)"]
        DRV_MOTOR["motor.c : PWM/方向输出控制"]
        DRV_GRAY["grayscale_sensor.c : 8路灰度引脚读取"]
        DRV_US["hcsr04.c : 超声波定时测距"]
        DRV_IMU["JY901P.c : 串口帧解析与偏航角解算"]
        DRV_UART["uart.c : 遥测与调试串口通信"]
    end

    subgraph Layer1["第 1 层：硬件抽象与系统配置 (Hardware Abstraction Layer)"]
        HAL["ti_msp_dl_config.c / .h (TI DriverLib)"]
    end

    MAIN --> KEY_APP
    MAIN --> TRACK_FSM
    TRACK_FSM --> LINE_PID
    LINE_PID -->|计算差速修正 correction| MOTOR_PID
    MOTOR_PID --> DRV_MOTOR
    TRACK_FSM --> DRV_GRAY
    MAIN -.-> DRV_US
    MAIN -.-> DRV_IMU
    MAIN --> DRV_UART

    DRV_MOTOR --> HAL
    DRV_GRAY --> HAL
    DRV_US --> HAL
    DRV_IMU --> HAL
    DRV_UART --> HAL
```

---

## 2. 核心运行机制与时间域划分

系统运行分为 **主循环 (后台任务)** 与 **中断服务程序 (前台任务)**：

### 2.1 前台高优先级任务 (中断域 Interrupt Domain)
- **GPIOA 双边沿中断 (`GROUP1_IRQHandler`)**：
  - 优先级：最高。
  - 任务：只要电机转动，编码器 A/B 相电平发生跳变即刻触发，通过状态跃迁查找表累加脉冲数。
- **10ms 定时器中断 (`pid_timer_INST_IRQHandler` on `TIMG0`)**：
  - 周期：严格 10ms。
  - 任务：禁止编码器中断进行快照读取，将 10ms 计数值与目标速度通过 PID 计算后，即刻输出新的左右轮 PWM 占空比。
- **1ms 定时器中断 (`key_timer_INST_IRQHandler` on `TIMG7`)**：
  - 周期：1ms（内部累加 10 次进入 10ms 消抖逻辑）。
  - 任务：按键消抖状态机更新。
- **100ms 超声波定时器中断 (`hcsr04_timer_INST_IRQHandler` on `TIMG6`)**：
  - 周期：100ms。
  - 任务：拉高 TRIG 引脚 10μs 发送测距声波。

### 2.2 后台任务 (主循环轮询 Background Loop in `init.c`)
1. 模式检测：扫描或获取当前按键选定的 `current_mode`；
2. 状态机清零：若检测到模式切换，调用 `condition_rst()` 复位寻迹状态机；
3. 执行具体模式任务（普通循迹、特殊赛道、摆动寻迹）；
4. 遥测回传：调用 `motor_encoder_condition()` 通过 UART0 输出电机速度与 PID 调试波形数据；
5. 适度延时 10ms 释放 CPU。
