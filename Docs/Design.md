# Mini Env Monitor Design

## 1. 项目简介

本项目基于 STM32F103C8T6 最小系统板实现迷你环境监测仪。系统使用 DHT11/DHT22 采集温湿度，通过 128x64 SSD1306 I2C OLED 显示运行状态，通过 USART1 每秒输出一条 JSON 数据，并使用 PA0 按键切换显示页面，PC13 板载 LED 指示正常或温度报警状态。

默认传感器类型为 DHT11。如需切换到 DHT22，可在 `Drivers/DHT/dht.h` 中将 `DHT_SENSOR_TYPE` 配置为 `DHT_TYPE_DHT22`。

## 2. 硬件连接表

| 模块 | STM32F103C8T6 引脚 | 连接说明 |
| --- | --- | --- |
| OLED SCL | PB6 | I2C1_SCL |
| OLED SDA | PB7 | I2C1_SDA |
| OLED VCC/GND | 3.3V/GND | 默认地址 0x3C，不显示时可在 `oled.h` 改为 0x3D |
| USART1 TX | PA9 | 接 USB-TTL RX |
| USART1 RX | PA10 | 可接 USB-TTL TX，也可仅接 TX 输出 |
| UART GND | GND | USB-TTL 与开发板共地 |
| DHT DATA | PB12 | 单总线数据脚，需上拉；模块自带上拉时可不额外加电阻 |
| DHT VCC/GND | 3.3V/GND | 推荐使用 3.3V 供电 |
| KEY | PA0 | 上拉输入，按下接 GND，低电平有效 |
| LED_STATUS | PC13 | Blue Pill 常见板载 LED，低电平点亮 |
| SWD | PA13/PA14 | ST-Link 下载调试专用，禁止占用 |

## 3. 软件分层结构

```text
main.c
  |
  +-- App/app.c
      |
      +-- Drivers/DHT
      +-- Drivers/OLED
      +-- Drivers/Button
      +-- Drivers/LED
      |
      +-- Utils/filter
      +-- Utils/uart_json
      |
      +-- CubeMX HAL handles: hi2c1, huart1, htim2
```

`main.c` 只保留 HAL 初始化、外设初始化、`App_Init()` 和 `App_Run()`。业务逻辑集中在 App 层，底层 GPIO、I2C、UART、TIM2 细节封装到驱动和工具模块。

## 4. 模块划分说明

| 模块 | 文件 | 职责 |
| --- | --- | --- |
| DHT 驱动 | `Drivers/DHT/dht.c/.h` | 使用 PB12 和 TIM2 us 计时读取 DHT11，保留 DHT22 解析结构 |
| OLED 驱动 | `Drivers/OLED/oled.c/.h` | SSD1306 I2C 初始化、显存缓存、字符串显示和刷新 |
| 字库 | `Drivers/OLED/oled_font.c/.h` | 5x7 ASCII 字模，自动将小写字母按大写显示 |
| Button | `Drivers/Button/button.c/.h` | PA0 低电平按下，20ms 消抖，短按事件只触发一次 |
| LED | `Drivers/LED/led.c/.h` | 封装 PC13 低电平点亮逻辑，App 不关心电平反向 |
| Filter | `Utils/filter.c/.h` | 固定窗口 5 的滑动平均滤波，无 malloc |
| UART JSON | `Utils/uart_json.c/.h` | 使用 `snprintf` 生成 JSON，`HAL_UART_Transmit` 输出 |
| App | `App/app.c/.h` | 非阻塞任务调度、页面切换、报警逻辑、数据上报 |

## 5. 状态说明

```text
正常显示
  |-- 每 10ms 扫描按键
  |-- 短按事件 --> 页面 0 / 页面 1 切换
  |-- 每 1000ms 读取 DHT
  |     |-- 成功: 更新滑动平均值，清除本次错误状态
  |     +-- 失败: dht_error_count + 1，保留上一次有效温湿度
  |-- 温度 > 30.0C --> 报警状态，LED 常亮
  +-- 温度 <= 30.0C --> 正常状态，LED 每秒翻转一次
```

页面 0 显示温度、湿度、运行时间、报警状态和 DHT 状态。页面 1 显示 MCU、UART、I2C、DHT 错误次数和运行时间。

## 6. 非阻塞任务调度

主循环不大量使用 `HAL_Delay()`，因为阻塞等待会让按键扫描、OLED 刷新和 UART 上报互相卡顿。`App_Run()` 每次进入时读取 `HAL_GetTick()`，通过时间差判断任务是否到期：

| 周期 | 任务 |
| --- | --- |
| 10ms | `Button_Update()` 扫描并消抖 |
| 500ms | 刷新 OLED 页面 |
| 1000ms | 读取一次 DHT |
| 1000ms | 输出一条 UART JSON |
| 1000ms | 正常状态下 LED 翻转 |

DHT 协议本身需要一次起始低电平和 us 级位宽测量，因此驱动内部存在必要的短等待，并且所有等待都有超时退出，避免传感器断线时长期卡死。

## 7. 按键消抖设计

PA0 配置为上拉输入，按下为低电平。驱动记录原始电平变化时间，只有电平保持稳定超过 20ms 才更新稳定状态。短按事件在“按下后释放”时产生，并由 `Button_GetShortPressEvent()` 读取后自动清除，保证一次短按只切换一次页面。

## 8. 滑动平均滤波设计

温度和湿度分别使用一个固定长度为 5 的滑动平均滤波器。读取成功后才把新数据送入滤波器；读取失败时保留上一次有效值，不强行输入 0，避免 OLED 和 JSON 数据突然跳变。

## 9. 对 AI 生成代码的修改或优化点

1. 将阻塞式主循环改为基于 `HAL_GetTick()` 的非阻塞调度，使按键、OLED、串口和 LED 任务按各自周期运行。
2. DHT 读取失败时保留上一次有效温湿度，只累计 `dht_error_count`，避免显示和 JSON 输出被错误清零。
3. LED 驱动层封装 PC13 低电平点亮差异，App 层只调用 `LED_On()`、`LED_Off()`、`LED_Toggle()`。
4. UART 和 OLED 的小数格式化没有直接依赖 `%f`，而是先转换为 0.1 单位整数，降低 Keil 工程中 printf 浮点支持配置导致的风险。

## 10. 开发关键问题和解决思路

| 问题 | 解决思路 |
| --- | --- |
| DHT 单总线时序严格 | TIM2 配置为 1us 计数，驱动内部用计数器测量响应和 bit 高电平宽度 |
| OLED 可能不显示 | 默认地址为 0x3C，如模块地址不同，在 `Drivers/OLED/oled.h` 改 `OLED_I2C_ADDRESS` 为 0x3D |
| PC13 LED 逻辑反向 | 在 LED 驱动中统一处理，低电平点亮，高电平熄灭 |
| DHT 失败导致界面数据异常 | App 层只在读取成功后更新滤波器，失败时保留旧值并累加错误次数 |

## 11. 编译和验证

工程为 Keil MDK 工程，打开 `MDK-ARM/bingjiang_STM32F103.uvprojx` 后直接 Build。新增源码已加入工程分组，新增 include path 已加入工程配置。

串口助手设置为 115200、8N1，连接 PA9 到 USB-TTL RX，GND 共地。正常运行后每秒可看到类似：

```json
{"temp":25.5,"hum":60.0,"time":120,"alarm":0}
```

若 DHT 读取失败，检查 3.3V 供电、GND、PB12 接线、DATA 上拉电阻，以及传感器类型是否与 `DHT_SENSOR_TYPE` 一致。若 LED 逻辑相反，只需要检查 `Drivers/LED/led.c` 中 `LED_On()` 和 `LED_Off()` 的写入电平是否符合实际板卡。

提交 GitHub 前建议整理步骤：

```text
1. 确认 Keil 可完整 Build。
2. 删除中间编译产物，只保留源码、工程文件、.ioc 和文档。
3. 新建 README，说明接线、编译、下载和串口验证。
4. git init
5. git add .
6. git commit -m "Add STM32 mini environment monitor"
7. 关联远程仓库并 push。
```
