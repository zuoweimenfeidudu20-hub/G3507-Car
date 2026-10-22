# 00 - AI 协作与维护指南 (AI Maintenance & Collaboration Protocol)

> **[IMPORTANT] 本规范对所有后续接手此项目的 AI（Gemini / Claude / GPT 等）及人类开发者具有强制约束力。**

---

## 1. 核心铁律：代码与文档强同步

任何 AI 助手在对本项目进行代码变更（包括但不限于：修改引脚分配、调整 PID 参数、修改状态机转移逻辑、新增/重构外设驱动）时，**必须在同一轮交互中同步更新 `docs/` 下对应的文档**。

禁止仅修改 `.c/.h` 代码而不更新文档的行为！

### 文档同步路由表：
| 代码变动类型 | 涉及的代码文件 | 必须同步更新的文档 |
| :--- | :--- | :--- |
| **引脚/定时器/外设配置** | `init.syscfg`, `ti_msp_dl_config.*` | `docs/01_HARDWARE_NETLIST.md` |
| **模块划分/系统调用层次** | `init.c`, `scripts/build.ps1` | `docs/02_SOFTWARE_ARCHITECTURE.md` |
| **电机控制/编码器算法/PID调参** | `init/motor.*`, `init/pid_motor.*` | `docs/03_CONTROL_ALGORITHMS.md` |
| **赛道识别/模式转移/按键状态机** | `init/grayscale_sensor.*`, `init/key.*` | `docs/04_STATE_MACHINES.md` |
| **模块导出函数与变量接口变更** | `init/*.h` | `docs/05_API_REFERENCE.md` |
| **缺陷排查/修复记录/新增任务** | 全工程 | `docs/06_TODO_AND_BUGS.md` |

---

## 2. 硬件引脚修改规范

1. **唯一事实源**：项目的硬件配置由 [init.syscfg](file:///c:/Users/34118/Desktop/WorkSpace/G3507-Car/init.syscfg) 管理，严禁在未经 SysConfig 配置的情况下在 C 代码中私自 hardcode 寄存器地址。
2. **网表同步**：若因硬件飞线或引脚冲突修改了引脚，必须：
   - 更新 `init.syscfg`；
   - 运行构建脚本生成新的 `ti_msp_dl_config.h`；
   - 更新 `docs/01_HARDWARE_NETLIST.md` 中的对应表格。

---

## 3. 中断处理规范 (MSPM0 GROUP 中断机制)

MSPM0 单片机的 GPIO 中断采用分组聚合机制：
- **`GROUP1_IRQHandler`** 聚合了 **GPIOA** 上的所有引脚中断。
- 若有多个模块使用 GPIOA 的引脚作为外部中断（例如编码器引脚与超声波 ECHO 引脚），**严禁在不同文件中重复定义 `GROUP1_IRQHandler`**，必须在统一的中断服务入口内，通过 `DL_GPIO_getEnabledInterruptStatus(GPIOA, ...)` 读取并分发中断标志。

---

## 4. 注释与可读性维护

1. 保留现有源码中的所有中文业务注释（尤其是同学对赛道模式、查表算法的思考注释）。
2. 在重构复杂算法时，使用数学公式或流程图说明变更理由。
