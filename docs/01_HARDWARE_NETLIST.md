# 01 - 硬件类网表与引脚映射 (Hardware Netlist & Pinout Matrix)

> **芯片型号**：TI MSPM0G3507 (LQFP-64 封装)  
> **时钟主频**：32 MHz / 80 MHz  
> **配置源文件**：[init.syscfg](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.syscfg)

---

## 1. 电机驱动与编码器网表 (Motor & Encoder)

| 逻辑网络名 | MCU 物理引脚 | 端口与外设通道 | I/O 模式 | 电气属性与中断 | 连接硬件与功能说明 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `MOTOR_DIR1_AIN1` | `PB9` | GPIOB Pin 9 | Output | Push-Pull | 右轮电机驱动输入 AIN1 |
| `MOTOR_DIR1_AIN2` | `PA12` | GPIOA Pin 12 | Output | Push-Pull | 右轮电机驱动输入 AIN2 |
| `MOTOR_DIR1_CH_1` | `PA16` | GPIOA Pin 16 | Input | 下拉，双边沿中断 | 右轮编码器 A 相 (CH1) |
| `MOTOR_DIR1_CH_2` | `PA15` | GPIOA Pin 15 | Input | 下拉，双边沿中断 | 右轮编码器 B 相 (CH2) |
| `MOTOR_DIR2_BIN1` | `PB0` | GPIOB Pin 0 | Output | Push-Pull | 左轮电机驱动输入 BIN1 |
| `MOTOR_DIR2_BIN2` | `PB16` | GPIOB Pin 16 | Output | Push-Pull | 左轮电机驱动输入 BIN2 |
| `MOTOR_DIR2_CH_11`| `PA22` | GPIOA Pin 22 | Input | 下拉，双边沿中断 | 左轮编码器 A 相 (CH1) |
| `MOTOR_DIR2_CH_22`| `PA17` | GPIOA Pin 17 | Input | 下拉，双边沿中断 | 左轮编码器 B 相 (CH2) |
| `PWM_motor_C0` | `PB4` | TIMA1 Channel 0 | PWM Output | 周期 1600 (约20kHz) | 右轮电机速度调宽 PWM |
| `PWM_motor_C1` | `PB1` | TIMA1 Channel 1 | PWM Output | 周期 1600 (约20kHz) | 左轮电机速度调宽 PWM |

---

## 2. 8路灰度红外寻迹传感器网表 (Grayscale Sensor Array)

传感器阵列自车身**最左侧到最右侧**排布，数字电平读取（1 表示检测到黑线，0 表示白色/反光地面）：

| 探头通道 | MCU 物理引脚 | 端口外设 | 算法位置权重 (weight) | 物理相对位置 |
| :--- | :--- | :--- | :--- | :--- |
| `GRAY1` | `PB15` | GPIOB Pin 15 | `-3500` | 最左翼探头 |
| `GRAY2` | `PA13` | GPIOA Pin 13 | `-2500` | 左偏外探头 |
| `GRAY3` | `PA14` | GPIOA Pin 14 | `-1500` | 左偏内探头 |
| `GRAY4` | `PB17` | GPIOB Pin 17 | `-500` | 中心左侧探头 |
| `GRAY5` | `PB18` | GPIOB Pin 18 | `+500` | 中心右侧探头 |
| `GRAY6` | `PB19` | GPIOB Pin 19 | `+1500` | 右偏内探头 |
| `GRAY7` | `PA21` | GPIOA Pin 21 | `+2500` | 右偏外探头 |
| `GRAY8` | `PB21` | GPIOB Pin 21 | `+3500` | 最右翼探头 |

---

## 3. 按键交互接口 (Key Buttons)

| 键位标识 | MCU 物理引脚 | 端口外设 | 电气配置 | 映射工作模式 |
| :--- | :--- | :--- | :--- | :--- |
| `PIN_key1` | `PB23` | GPIOB Pin 23 | 内部上拉，低电平按下，双边沿中断 | `key1_putong` (普通循迹模式) |
| `PIN_key2` | `PB8` | GPIOB Pin 8 | 内部上拉，低电平按下，双边沿中断 | `key2_teshu` (特殊赛道/台阶模式) |
| `PIN_key3` | `PB3` | GPIOB Pin 3 | 内部上拉，低电平按下，双边沿中断 | `key3_baidong` (摆动循线模式) |
| `PIN_key4` | `PB13` | GPIOB Pin 13 | 内部上拉，低电平按下，双边沿中断 | `key_idle` (急停/待机模式) |

---

## 4. 辅助传感器与通信网表 (Sensors & Interfaces)

### 4.1 HC-SR04 超声波测距
- **`HCSR04_TRIG`**：`PB14`，GPIO 输出，输出 10μs 高电平触发测距。
- **`HCSR04_ECHO`**：`PA1`，GPIO 输入，配置双边沿中断，测量回响高电平脉宽。
- **相关定时器**：`TIMG6` (`hcsr04_timer`)，预分频 8/8 (500kHz)，周期 100ms。

### 4.2 JY901P 航姿参考系统 (IMU)
- **`UART1_RX`**：`PA18`，接模块 TX，9600 bps，8N1，中断接收姿态角数据包。
- **`UART1_TX`**：`PA8`，接模块 RX。

### 4.3 调试串口 (Debug Telemetry)
- **`UART0_TX`**：`PA10`，115200 bps，上位机调参/编码器波形打印。
- **`UART0_RX`**：`PA11`，115200 bps。

### 4.4 扩展外设预留
- **0.96寸 OLED**：`I2C0` (SDA: `PA28`, SCL: `PA31`), 100 kHz 速率。
- **舵机 PWM**：`TIMA0` (CH0: `PA0`, CH1: `PB20`), 50Hz 周期 (20ms)。
- **火焰传感器 ADC**：`ADC0` 5通道序列转换 (`PA27, PA26, PA25, PA24, PB25`)。

---

## 5. 定时器与系统中断向量映射表

| 定时器实例 | SysConfig 别名 | 分频与主频 | 设定周期 | 中断服务函数 | 负责任务 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `TIMG0` | `pid_timer` | 32MHz / 8 / 4 = 1MHz | 10 ms | `pid_timer_INST_IRQHandler` | 电机速度闭环计算与 PWM 更新 |
| `TIMG6` | `hcsr04_timer` | 32MHz / 8 / 8 = 500kHz | 100 ms | `hcsr04_timer_INST_IRQHandler`| 超声波 100ms 定周期测距触发 |
| `TIMG7` | `key_timer` | 32MHz / 8 / 8 = 500kHz | 1 ms | `key_timer_INST_IRQHandler` | 每 1ms 扫描按键，连续 30ms 确认按下/松开 |
| `TIMG8` | `track_timer` | 32MHz / 8 / 8 = 500kHz | 40 ms | `track_timer_INST_IRQHandler`| 灰度采样；主循环消费采样并执行循迹状态机 |
| `TIMA1` | `PWM_motor` | 周期 1600 | ~20 kHz | 无中断 (纯硬件PWM) | 驱动双路直流电机 |
| `TIMA0` | `PWM_servo` | 周期 20000 | 50 Hz | 无中断 (纯硬件PWM) | 驱动双路舵机 |
| `GPIOA` | `GPIO_MULTIPLE_GPIOA` | - | 边沿捕获 | `GROUP1_IRQHandler` | 编码器双边沿计数 (+ 超声波ECHO) |
