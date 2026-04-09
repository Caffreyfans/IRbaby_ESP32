# IRbaby ESP32 🔴 万能红外遥控固件

![Build Status](https://github.com/caffreyfans/IRbaby_ESP32/actions/workflows/main.yml/badge.svg)
![License](https://img.shields.io/badge/license-MIT-green)
![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.4.3-blue)

## 📖 项目描述

基于 **ESP-IDF v5.4.3** 开发的万能红外遥控固件，支持多种智能家居生态。通过直观的网页前端完成配置，无需编程即可控制各类红外设备。

### ✨ 核心特性

- 🌐 **网页配置界面** - 友好的Web UI，支持实时控制和配置
- 🏠 **Home Assistant 集成** - 自动发现并集成到 HA 气候控制系统
- 🍎 **HomeKit 集成** - 接入苹果HomeKit生态，支持自定义配件
- 🔗 **MQTT 支持** - 与 Home Assistant 等平台无缝集成，支持自动发现
- ☁️ **OTA 升级** - 支持空中升级固件
- 💾 **文件管理** - 支持通过Web界面上传和管理配置文件
- 🔄 **远程控制** - 通过MQTT和Web API控制红外信号发送

## 🔧 支持硬件

| 芯片 | 支持 | 说明 |
|------|------|------|
| ESP32 | ✅ | 推荐 |
| ESP32-C6 | ✅ | 支持 |
| ESP32-S2 | ✅ | 支持 |
| ESP32-C3 | ✅ | 支持 |
| ESP32-S3 | ✅ | 支持 |
| ESP32-C2 | ❌ | 不支持 |
| ESP32-H2 | ❌ | 不支持 |
| ESP32-P4 | ❌ | 不支持 |

## 🚀 快速开始

### 环境要求

- ESP-IDF v5.4.3 或更高版本
- Python 3.8+
- Git

### 克隆项目

```bash
git clone https://github.com/Caffreyfans/IRbaby_ESP32.git
cd IRbaby_ESP32
git submodule update --init --recursive
```

### 编译固件

```bash
idf.py build
```

### 烧录固件

```bash
idf.py flash monitor
```

## 📋 功能清单

### 已完成 ✅

- [x] 网页UI配置界面
- [x] MQTT 控制与 HomeAssistant 集成- [x] Home Assistant 自动发现与气候控制- [x] HomeKit 配件支持
- [x] OTA 远程升级
- [x] 文件管理功能
- [x] 系统重启控制

### 待计划 📌

- [ ] 官方 HomeKit SDK 集成
- [ ] HTTPS/SSL 加密传输
- [ ] 用户认证与权限管理
- [ ] 二进制文件优化

## 📁 项目结构

```
IRbaby_ESP32/
├── main/                 # 主应用程序
│   ├── homekit.c        # HomeKit 配件管理
│   ├── mqtt.c           # MQTT 客户端
│   ├── web_handler.c    # HTTP 请求处理
│   ├── ir.c             # 红外信号处理
│   └── conf.c           # 配置管理
├── components/          # 第三方组件
│   └── IRbaby/          # IRbaby SDK
├── build/               # 编译输出
└── CMakeLists.txt       # CMake 配置
```

## 🔌 API 接口

### Web API

- `GET /` - Web UI首页
- `GET /api/config` - 获取当前配置
- `POST /api/config` - 更新配置
- `POST /api/homekit` - HomeKit 配件信息
- `GET /api/files` - 文件列表
- `POST /api/upload` - 上传文件
- `POST /api/reboot` - 系统重启

### MQTT 主题

- `homeassistant/climate/{device_id}/config` - HA 自动发现配置
- `{prefix}/state` - 设备状态 (HA 兼容格式)
- `{prefix}/command` - 控制命令 (支持 HA 气候控制)
- `{prefix}/response` - 命令响应
- `homekit/state` - HomeKit 状态 (向后兼容)
- `ir/command` - 红外命令控制 (向后兼容)
- `ir/status` - 红外状态反馈 (向后兼容)

## 📝 配置示例

通过Web界面配置 MQTT：

```json
{
  "mqtt_enable": true,
  "mqtt_broker": "192.168.1.100",
  "mqtt_port": 1883,
  "mqtt_topic_prefix": "irbaby"
}
```

## 🏠 Home Assistant 集成

IRbaby ESP32 支持自动发现并集成到 Home Assistant 的气候控制系统。

### 自动发现

设备连接到 MQTT 后会自动发布发现消息，HA 会自动识别并添加气候控制实体。

### 支持的功能

- **模式控制**: off, auto, cool, heat, dry, fan_only
- **温度控制**: 16-30°C 范围，1°C 步进
- **风速控制**: auto, low, medium, high
- **状态反馈**: 实时状态同步到 HA

### MQTT 消息格式

**状态消息** (发布到 `{prefix}/state`):
```json
{
  "mode": "cool",
  "temperature": 24,
  "fan_mode": "auto"
}
```

**控制命令** (订阅 `{prefix}/command`):
```json
{
  "mode": "heat",
  "temperature": 26,
  "fan_mode": "medium"
}
```

### 配置步骤

1. 在 IRbaby Web UI 中配置 MQTT 连接
2. 确保 HA 已配置相同的 MQTT Broker
3. 重启 IRbaby 设备，等待自动发现完成
4. 在 HA 中查看新添加的气候控制实体

## 🐛 故障排除

### 无法连接WiFi

1. 检查 WiFi 密码是否正确
2. 确保ESP32在路由器信号范围内
3. 尝试重启设备

### MQTT 连接失败

1. 验证MQTT Broker地址和端口
2. 检查防火墙设置
3. 查看系统日志获取详细错误信息

## 📄 许可证

MIT License - 详见 [LICENSE](LICENSE) 文件

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📧 联系方式

如有问题或建议，欢迎提交 [Issues](https://github.com/Caffreyfans/IRbaby_ESP32/issues)