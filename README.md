# MSPM0G3507 智能小车工程

> **[IMPORTANT] AI 协作维护总纲**：  
> 本工程已完成模块化文档拆分。任何接手本项目的 AI（或协作者）在修改程序、调整引脚或重构代码时，**必须根据修改类型同步更新 [docs/](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs) 目录下对应的细分文档**！  
> 详情请先阅读：[00 - AI 维护与协同指南](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\00_AI_MAINTENANCE_GUIDE.md)。

---

## 一、 项目概览

本项目基于 **德州仪器 TI MSPM0G3507** (ARM Cortex-M0+) 微控制器构建，实现了一台具备**双轮差速闭环驱动、8路加权灰度循迹、超声波避障、九轴IMU航向解算与按键多模式调度**的智能循迹小车。

- **主控配置**：[init.syscfg](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.syscfg) (TI SysConfig v1.26.2)
- **编译工具链**：TI Arm Clang (`ti-cgt-armllvm`) + [scripts/build.ps1](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\scripts\build.ps1)
- **下载与调试**：TI XDS110 配合 OpenOCD 完成烧录和 VS Code 调试（`openocd/xds110-mspm0g3507.cfg`）。
- **可视化看板**：[docs/dashboard.html](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\dashboard.html)（双击在浏览器中直观查看引脚网表与控制流）
- **核心伪代码**：[应用层状态机切换逻辑_伪代码.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\应用层状态机切换逻辑_伪代码.c)（全中文条件判断与状态机跳转逻辑骨架）

---

## 二、 模块化文档索引 (Documentation Index)

为了便于人类开发者与 AI 快速定位特定模块、降低单次阅读上下文损耗，项目文档已按功能拆分为以下独立文件：

| 序号 | 文档名称 | 对应核心内容与用途 | 重点关注文件 |
| :---: | :--- | :--- | :--- |
| **00** | [AI 维护与协同指南](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\00_AI_MAINTENANCE_GUIDE.md) | **AI 必读**：代码修改必须同步的文档映射表、中断与引脚修改铁律 | 全局规范 |
| **01** | [硬件类网表与引脚映射](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\01_HARDWARE_NETLIST.md) | 左右电机、编码器、8路灰度、按键、超声波/陀螺仪物理引脚与定时器分配 | [init.syscfg](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.syscfg) |
| **02** | [软件分层架构与调用流](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\02_SOFTWARE_ARCHITECTURE.md) | 业务层、算法层、驱动层、HAL层四层划分图与前台中断/后台轮询时间域 | [init.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init.c) |
| **03** | [运动闭环与控制算法](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\03_CONTROL_ALGORITHMS.md) | 编码器4倍频状态机查表解码、10ms 速度环 PID、加权灰度转向环偏差解算 | [pid_motor.c](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\init\pid_motor.c) |
| **04** | [状态机体系与场景决策](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\04_STATE_MACHINES.md) | **赛题核心决策**：三层分层状态机 (HSM)、楼梯跑满2圈闭环仲裁、直道避障/正弦摆动与恒速保障 | [grayscale_sensor.c](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init/grayscale_sensor.c) |
| **05** | [核心 API 接口与全局符号表](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\05_API_REFERENCE.md) | 所有 `.h` 头文件导出的函数接口原型、输入输出参数及关键全局变量清单 | [init/*.h](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init) |
| **06** | [已知缺陷诊断与路线图](file:///C:\Users\34031\Desktop\mpsm0\outputs\MSPM0G3507-second_process\docs\06_TODO_AND_BUGS.md) | **调试必读**：GPIOA 共享中断冲突（超声波ECHO）、主循环未接通寻迹等核心隐患 | 待修复缺陷 |

---

## 三、 快速构建与烧录

通过 PowerShell 运行根目录下的脚本：

```powershell
# 1. 编译全工程（自动调用 SysConfig 生成配置代码并使用 tiarmclang 编译）
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Action Build

# 2. 打开 SysConfig 图形化引脚配置工具
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Action SysConfig

# 3. 使用 XDS110 烧录已编译的固件，并校验和复位
powershell -ExecutionPolicy Bypass -File .\scripts\probe.ps1 -Action Flash
```

VS Code 中运行 `MSPM0: Build and flash (XDS110)` 可先编译再烧录。按 F5 选择 `MSPM0G3507 - XDS110 build, flash and debug`，会先编译，再由 XDS110 烧录并进入调试，自动停在 `main()`；设置断点后可继续运行或单步检查。始终使用同一支探针。

连接检查：`probe.ps1 -Action Check` 检查 XDS110；`-Action CheckWireless` 可检查无线 DAP。`-Action CheckXds110` 保留为 XDS110 检查的兼容写法。

---

## 四、 源码目录结构

```text
G3507-Car/
├── README.md                      # [本项目入口导航与全局说明]
├── 应用层状态机切换逻辑_伪代码.c  # [应用层主线与全中文状态机判断骨架]
├── docs/                          # [模块化细分文档库]
│   ├── 00_AI_MAINTENANCE_GUIDE.md # AI 协同规则与维护路由表
│   ├── 01_HARDWARE_NETLIST.md     # 硬件类网表与引脚映射
│   ├── 02_SOFTWARE_ARCHITECTURE.md# 软件四层架构与调用流
│   ├── 03_CONTROL_ALGORITHMS.md   # 速度/转向闭环控制与解码算法
│   ├── 04_STATE_MACHINES.md       # 赛道识别与动作状态机
│   ├── 05_API_REFERENCE.md        # 头文件导出函数与全局符号
│   ├── 06_TODO_AND_BUGS.md        # 已知致命缺陷与修复方案
│   └── dashboard.html             # 全景可视化类网表与控制看板
├── init.c                         # 主程序入口 (main 轮询状态机)
├── init.syscfg                    # TI SysConfig 硬件配置文件
├── init/                          # 核心驱动与控制算法源码
│   ├── motor.c / .h               # 电机底层驱动与 PWM 占空比输出
│   ├── pid_motor.c / .h           # 编码器中断 4 倍频查表 & 10ms 速度环 PID
│   ├── grayscale_sensor.c / .h    # 8路灰度加权解算与赛道状态机
│   ├── key.c / .h                 # 按键状态机消抖与模式切换
│   ├── uart.c / .h                # 调试串口遥测与数据打包
│   ├── JY901P.c / .h              # 九轴陀螺仪串口数据接收与偏航角解算
│   ├── hcsr04.c / .h              # 超声波测距定时触发与脉宽捕获
│   ├── hcsr04_internal.h          # 超声波中断内部钩子声明
│   └── delay.c / .h               # 延时函数
└── scripts/                       # 编译构建与调试探针脚本
```
