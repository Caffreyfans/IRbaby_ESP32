![Build Status](https://github.com/caffreyfans/IRbaby_ESP32/actions/workflows/main.yml/badge.svg)

## 仓库描述：
基于 esp-idf-v5.4.1 开发的万能红外遥控固件，可通过网页前端完成配置。

## 支持硬件
|HardWare|ESP32|ESP32S2|ESP32C3|ESP32S3|ESP32C2|ESP32C6|ESP32H2|ESP32P4|ESP32C5|ESP32C61|ESP8266|
|---|---|---|---|---|---|---|---|---|---|---|---|
|Support|✔️|✔️|✔️|✔️|✔️|✔️|✔️|✔️|✔️|✔️|❌|

**开发计划**
- [ ] 支持 OTA 升级
- [ ] 支持 MQTT 控制，接入 HomeAssistant
- [ ] 接入 HomeKit

## 拉取
```shell
git clone https://github.com/Caffreyfans/IRbaby_ESP32.git
git submodule update --init --recursive
```