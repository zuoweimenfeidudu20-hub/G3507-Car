# 03 - 运动闭环与控制算法 (Control Algorithms & Closed-Loop)

---

## 1. 电机编码器 4 倍频状态跃迁查表法

在 [init/pid_motor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\pid_motor.c) 中，编码器解码摒弃了繁重的浮点和复杂的 `if-else` 分支，采用微控制器高效率的 **状态转移查找表 (State Transition Look-Up Table)**：

### 1.1 编码逻辑
- 组合 4 位状态码：`Index = (History_AB << 2) | Current_AB`
- 其中高 2 位为上一次采样状态，低 2 位为当前采样状态；A 相为高位、B 相为低位。
- 转移表定义：
  ```c
  static const int8_t encoder_transition[16] = {
        0,  1, -1,  0,
       -1,  0,  0,  1,
        1,  0,  0, -1,
        0, -1,  1,  0
  };
  ```
- **非法跳变检测**：若 `(History_AB ^ Current_AB) == 3`（A、B 相同时反转，在物理上不可能），计入 `error_count` 过滤高频毛刺干扰。

---

## 2. 10ms 速度闭环 PID 控制器

### 2.1 目标值转换
- 输入为目标占空比百分比 `-100 ~ +100`；
- 通过 `PercentToCounts(percent)` 将其转换为 10ms 周期内的目标脉冲计数值：
  `TargetCounts = (|percent| × max_count_10ms + 50) / 100`
  （目前定义 `max_count_10ms = 58`）。

### 2.2 PID 计算与抗饱和机制
```mermaid
flowchart TD
    A[输入: 目标计数值 target & 测量计数值 measured] --> B[计算偏差: error = target - measured]
    B --> C{目标方向与上次同向?}
    C -- 否(换向) --> D[积分清零: integral=0, 上次误差=0]
    C -- 是 --> E[计算微分量: derivative = error - prev_error]
    D --> E
    E --> F[积分项累加 & 限幅: Clamp -integral_limit ~ +integral_limit]
    F --> G[计算 PID 输出: (Kp*error + Ki*integral + Kd*derivative) / 100]
    G --> H[综合输出 = 目标值 + correction, 钳位到 -100 ~ 100]
    H --> I{达到饱和输出且误差继续累加?}
    I -- 是(抗饱和) --> J[回退积分值: integral = previous_integral]
    I -- 否 --> K[保存历史量: prev_error = error]
    J --> K
    K --> L[输出 PWM 占空比到电机驱动]
```

- **增益常量**：
  - `motor_kp = 250`
  - `motor_ki = 3`
  - `motor_kd = 0`
  - 缩放基准 `pid_motor_gain_scale = 100`

---

## 3. 8路加权灰度转向环控制

### 3.1 线性加权横向偏差解算
灰度传感器阵列由 8 个数字探头组成，分别配置权值：
`W = [-3500, -2500, -1500, -500, 500, 1500, 2500, 3500]`
当探头检测到黑线时值为 1，检测到白色地面时为 0。

系统计算当前横向加权平均重心：
`error = Σ(i=0…7)(W_i × V_i) / Σ(i=0…7)V_i`
- 若 `error < 0`，说明小车偏右，黑线偏左，需要向左修正；
- 若 `error > 0`，说明小车偏左，黑线偏右，需要向右修正；
- 若 `ΣV_i = 0`，判定为**丢线 (pattern_lost)**。

### 3.2 差速分配逻辑
在 [init/grayscale_sensor.c](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/grayscale_sensor.c) 的 `set_target(pwm, correction)` 中：
- `Left_Target = Clamp(base_pwm + correction, 0, 100)`
- `Right_Target = Clamp(base_pwm - correction, 0, 100)`
基准速度默认 `base_pwm = 35`，通过调整 `correction` 实现平滑差速转弯。
