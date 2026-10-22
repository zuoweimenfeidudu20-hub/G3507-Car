# 05 - 核心 API 接口与全局符号表 (API Reference & Global Symbols)

---

## 1. 电机驱动模块 ([init/motor.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/motor.h))

| 函数声明 | 参数 | 返回值 | 作用描述 |
| :--- | :--- | :--- | :--- |
| `void rightmotor_output(int32_t output)` | `output: -100 ~ +100` | `void` | 设置右电机输出（正数正转，负数反转，内部自动钳位） |
| `void leftmotor_output(int32_t output)`  | `output: -100 ~ +100` | `void` | 设置左电机输出（正数正转，负数反转，内部自动钳位） |
| `void allstop(void)`                     | 无 | `void` | 左右电机同时断电完全刹停 |
| `int32_t motor_clamp(int32_t percent)`   | `percent` | `int32_t` | 占空比百分比安全限幅至 `[-100, 100]` |

---

## 2. 闭环控制模块 ([init/pid_motor.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/pid_motor.h))

### 2.1 核心函数
| 函数声明 | 作用描述 |
| :--- | :--- |
| `int pid_motor(PIDMotor_Controller *c, int target_pct, int target_cnt, int meas_cnt)` | 单轮速度 PID 计算，输入期望脉冲和测量脉冲，返回 PWM 占空比 |
| `int32_t PercentToCounts(int32_t percent)` | 将目标速度百分比（0~100%）折算为 10ms 目标编码器脉冲数 |
| `void pidmotor_rst(PIDMotor_Controller *controller)` | 重置 PID 控制器的积分项与历史误差 |

### 2.2 关键全局变量
| 变量名 | 类型 | 说明 |
| :--- | :--- | :--- |
| `right_encoder_count` | `volatile int32_t` | 右编码器脉冲累加器 (由 GPIOA 中断更新) |
| `left_encoder_count`  | `volatile int32_t` | 左编码器脉冲累加器 (由 GPIOA 中断更新) |
| `right_count_10ms`    | `volatile int` | 右轮 10ms 采样周期实际脉冲数 (快照) |
| `left_count_10ms`     | `volatile int` | 左轮 10ms 采样周期实际脉冲数 (快照) |
| `init_rightpwm`       | `volatile int` | 当前设定的右轮基准目标百分比 (默认 35) |
| `init_leftpwm`        | `volatile int` | 当前设定的左轮基准目标百分比 (默认 35) |

---

## 3. 灰度寻迹模块 ([init/grayscale_sensor.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/grayscale_sensor.h))

| 函数声明 | 作用描述 |
| :--- | :--- |
| `void sensor_read(int values[8])` | 读取 8 路灰度引脚电平填入数组 |
| `trackpattern track_analyze(int values[8], int *error)` | 计算当前赛道特征与横向加权偏差值 `error` |
| `void track_controll(void)` | 寻迹主控制函数（包含赛道状态机与差速输出） |
| `int line_pid(track_pid *pid, int error)` | 转向纠偏 PID，计算差速调整量 `correction` |
| `void set_target(int pwm, int correction)` | 设置左右轮基准速度与差速修正 |
| `void condition_rst(trackstate next)` | 状态机重置，并指定下一个目标状态 |
| `bool track_confirm(bool condition, int require)` | 状态防抖滤波确认函数 |

---

## 4. 按键模块 ([init/key.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/key.h))

| 函数声明 | 作用描述 |
| :--- | :--- |
| `void keyscan(void)` | 主循环轮询式消抖扫描 |
| `void key_tick10ms(void)` | 10ms 定时器中断驱动式消抖处理 |
| `keymode key_getmode(void)` | 获取当前选定的运行模式枚举 (`key1_putong`, `key2_teshu`, `key3_baidong`, `key_idle`) |

---

## 5. 扩展传感器接口
- **超声波** ([init/hcsr04.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/hcsr04.h))：
  - `void HCSR04_Init(void)`：启动 100ms 自动测距。
  - `float HCSR04_GetDistance(void)`：获取最近有效距离（厘米）。
  - `void HCSR04_HandleEchoInterrupt(void)`：ECHO 引脚捕获回调（在 [init/hcsr04_internal.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/hcsr04_internal.h) 声明）。
- **九轴陀螺仪** ([init/JY901P.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/JY901P.h))：
  - `void JY901P_Init(void)`：初始化串口通信。
  - `bool JY901P_GetYaw(float *yaw_degrees)`：非阻塞读取航向偏航角（-180° ~ +180°）。
  - `bool JY901P_GetGyroZ(float *gyro_z_dps)`：读取 Z 轴旋转角速度。
