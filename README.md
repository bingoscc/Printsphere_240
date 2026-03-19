# PrintSphere 240

原神神之眼 打印机伴侣 ESP32-S3 移植版

## 硬件平台

| 项目 | 型号/规格 |
|------|-----------|
| 主控芯片 | ESP32-S3 (Xtensa dual-core 32-bit) |
| 显示屏 | 240x240 SPI TFT (GC9A01 圆形) |
| 存储 | SDMMC SD卡 |
| WiFi | ESP32-S3 内置 WiFi (AP+STA 模式) |

### 硬件接口定义

| 功能 | GPIO | 说明 |
|------|------|------|
| TFT_MISO | -1 | 未连接 |
| TFT_SCK | GPIO12 | SPI时钟 |
| TFT_MOSI | GPIO13 | SPI数据 |
| TFT_CS | GPIO11 | 片选 |
| TFT_DC | GPIO10 | 数据/命令 |
| TFT_RST | GPIO21 | 复位 |
| TFT_BLK | GPIO14 | 背光控制 |
| BTN_PIN | GPIO38 | 按键输入 |
| SDMMC_CMD | GPIO42 | SD卡命令 |
| SDMMC_CLK | GPIO41 | SD卡时钟 |
| SDMMC_D0 | GPIO40 | SD卡数据 |

## 软件框架

| 组件 | 版本 | 说明 |
|------|------|------|
| ESP-IDF | v5.3.x | ESP32 开发框架 |
| LVGL | v8.3.x | 图形显示库 |
| cJSON | latest | JSON 解析库 |
| MQTT | ESP-IDF mqtt | MQTT 客户端 |

### ESP-IDF 组件依赖

```
esp32
lvgl
esp_timer
esp_netif
mqtt
json (cJSON)
nvs_flash
wifi_provisioning
driver (GPIO, SPI)
```

## 功能特性

### P0 - 核心功能

- [x] **WiFi Manager** - AP+STA 双模式，支持配网
- [x] **Config Store** - NVS 持久化配置存储
- [x] **Bambu Cloud Client** - 云端 MQTT 通信，登录认证
- [x] **Printer Client** - 本地打印机 HTTP 状态轮询
- [x] **UI 显示层** - LVGL 适配 240x240 显示
- [x] **单按键交互** - 短按/长按/双击操作

### P1 - 重要功能

- [x] **Setup Portal** - Web 配网页面 (80端口)
- [x] **状态显示** - 打印进度、温度、剩余时间
- [x] **HASS MQTT** - HomeAssistant MQTT 发现协议集成

## 项目结构

```
printsphere_240/
├── CMakeLists.txt                    # ESP-IDF 项目根配置
├── sdkconfig.defaults                # ESP-IDF 默认配置
├── partitions.csv                    # 分区表
├── main/
│   ├── CMakeLists.txt               # 主组件构建配置
│   ├── Kconfig.projbuild            # 项目配置菜单
│   ├── app_main.cpp                # 应用入口
│   ├── board_pins.h                # 硬件引脚定义
│   ├── display/                     # 显示驱动
│   │   ├── tft_drivers.h
│   │   └── tft_drivers.cpp         # GC9A01 驱动
│   ├── ui/                          # LVGL UI
│   │   ├── ui.h/.cpp               # UI 主逻辑
│   │   ├── screens.h/.cpp          # 屏幕定义
│   │   ├── progress_ring.h/.cpp     # 进度环控件
│   │   └── styles.h/.cpp          # 样式定义
│   ├── input/                       # 输入处理
│   │   ├── button.h/.cpp           # 按键处理
│   ├── cloud/                       # 云端通信
│   │   ├── bambu_cloud_client.h/.cpp  # Bambu Cloud MQTT
│   │   └── printer_client.h/.cpp    # 本地打印机
│   ├── hass/                        # HomeAssistant
│   │   └── hass_mqtt.h/.cpp        # MQTT 发现集成
│   ├── wifi/                         # WiFi 管理
│   │   └── wifi_manager.h/.cpp
│   ├── config/                       # 配置存储
│   │   └── config_store.h/.cpp
│   ├── http/                         # Web 服务
│   │   └── setup_portal.h/.cpp     # 配网页面
│   └── state/                        # 状态管理
│       └── printer_state.h/.cpp
```

## 编译构建

### 环境要求

- ESP-IDF v5.3.x
- Python 3.8+
- 支持 Linux/Windows/macOS

### 构建步骤

```bash
# 1. 克隆仓库
git clone https://github.com/bingoscc/Printsphere_240.git
cd Printsphere_240

# 2. 安装 ESP-IDF (如未安装)
# 参考: https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32s3/get-started/index.html

# 3. 激活 ESP-IDF 环境
. $IDF_PATH/export.sh  # Linux/macOS
# 或
% %IDF_PATH%\export.bat  # Windows

# 4. 设置目标芯片
idf.py set-target esp32s3

# 5. 配置项目 (可选)
idf.py menuconfig

# 6. 编译
idf.py build

# 7. 烧录
idf.py -p /dev/ttyUSB0 flash

# 8. 监视输出
idf.py monitor
```

### 分区配置

| 分区 | 大小 | 用途 |
|------|------|------|
| nvs | 24KB | WiFi配置等 |
| spiffs | 64KB | SPIFFS文件系统 |
| app0 | 3.3MB | 应用程序 |

## 配置说明

### 首次配置

设备首次启动会自动创建 AP `PrintSphere-Setup`，连接后访问 http://192.168.4.1 进行配置：

1. **WiFi 设置** - 输入路由器 SSID 和密码
2. **Bambu Lab** - 填入账号邮箱、密码、设备序列号
3. **HomeAssistant** - MQTT 服务器地址和端口

### 手动配置

可通过 `idf.py menuconfig` 修改默认配置：

```
PrintSphere Configuration
├── Device Name
├── Setup AP SSID
├── Enable Bambu Cloud
├── Enable HomeAssistant MQTT
└── Printer Poll Interval (ms)
```

## 按键操作

| 操作 | 功能 |
|------|------|
| 短按 (300-1000ms) | 切换视图 |
| 长按 (>1000ms) | 进入设置页面 |
| 双击 | 重启设备 |

## MQTT 主题

### HomeAssistant 发现主题

```
homeassistant/sensor/{device_id}/printer_state/config
homeassistant/sensor/{device_id}/progress/config
homeassistant/sensor/{device_id}/nozzle_temp/config
homeassistant/sensor/{device_id}/bed_temp/config
homeassistant/sensor/{device_id}/remaining_time/config
```

### 状态发布主题

```
printsphere/{device_id}/state
```

### 状态 JSON 格式

```json
{
  "status": "Printing",
  "progress": 45.5,
  "nozzle_temp": 215.0,
  "nozzle_target": 220.0,
  "bed_temp": 60.0,
  "bed_target": 60.0,
  "remaining_time": 3600,
  "job_name": "model.gcode"
}
```

## Bambu Cloud API

| 端点 | 说明 |
|------|------|
| https://api.bambulab.com/v1/auth/login | 用户认证 |
| mqtt.bambulab.com:8883 | MQTT 服务器 |

### MQTT 订阅主题

```
device/{serial}/report
```

## 显示屏规格

### GC9A01 (圆形)

| 参数 | 值 |
|------|-----|
| 分辨率 | 240x240 |
| 颜色深度 | RGB565 (16-bit) |
| SPI 模式 | Mode 0 |
| SPI 频率 | 40MHz |
| 驱动IC | GC9A01 |

## 已知问题

1. Setup Portal 的 /save 接口暂未完整实现 JSON 保存
2. Bambu Cloud 完整协议解析待完善
3. LVGL 进度环使用简化绘制算法

## 许可证

MIT License

## 参考项目

- [PrintSphere](https://github.com/guylanghammer/printsphere) - 原版 Bambu Lab 打印机伴侣
- [genshin_godeye](https://github.com/planevina/genshin_godeye) - 原神神之眼硬件设计
- [LvGL](https://github.com/lvgl/lvgl) - Light and Versatile Graphics Library
- [ESP-IDF](https://github.com/espressif/esp-idf) - Espressif IoT Development Framework

## 版本历史

### v1.0.0 (2026-03-19)

- 初始版本
- 支持 GC9A01 240x240 TFT
- 支持 Bambu Cloud MQTT 连接
- 支持 HomeAssistant MQTT 发现协议
- Web 配网界面
