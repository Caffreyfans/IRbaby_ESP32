# Release v0.6: Home Assistant MQTT Discovery & Climate Control

## 🚀 What's New

This release introduces full Home Assistant integration with automatic MQTT discovery and climate control support, making IRbaby ESP32 a seamless addition to any smart home setup.

### ✨ Key Features

- **🏠 Home Assistant Auto-Discovery**: IRbaby automatically appears as a climate entity in Home Assistant
- **🌡️ Full Climate Control**: Support for all major AC modes (cool, heat, auto, dry, fan_only)
- **🌬️ Fan Speed Control**: Complete fan speed management (auto, low, medium, high)
- **🌡️ Temperature Control**: Precise temperature setting (16-30°C range)
- **🔄 Real-time State Sync**: Bidirectional communication with instant status updates
- **🛡️ Thread-Safe Operations**: Enhanced stability with mutex-protected MQTT operations

### 🔧 Technical Improvements

- **Memory Management**: Fixed memory leaks and improved resource cleanup
- **Error Handling**: Enhanced error reporting and recovery mechanisms
- **Partition Optimization**: Adjusted flash partition sizes for better space utilization
- **Code Quality**: Resolved compilation warnings and improved code structure

### 📚 Documentation Updates

- **README Enhancement**: Added comprehensive Home Assistant integration guide
- **API Documentation**: Updated MQTT topic specifications
- **Configuration Examples**: Added HA-specific configuration samples

## 📦 Release Assets

- `ir_protocols.bin` - ESP32 firmware binary (1.16MB)
- Source code with complete Home Assistant integration

## 🔄 Migration Guide

### For Existing Users

1. **Backup Configuration**: Save your current settings via Web UI
2. **Flash New Firmware**: Use `idf.py flash` or your preferred flashing tool
3. **Reconfigure MQTT**: Ensure MQTT settings are correct in Web UI
4. **Restart Device**: Allow auto-discovery to complete in Home Assistant

### For New Users

1. **Flash Firmware**: Burn the new firmware to your ESP32
2. **Initial Setup**: Configure WiFi and MQTT through Web UI
3. **Home Assistant**: Wait for automatic discovery or manually add MQTT climate entity

## 🐛 Bug Fixes

- Fixed MQTT client undeclared variable errors
- Resolved memory management issues in MQTT operations
- Corrected partition table sizing for optimal performance
- Fixed compilation warnings and errors

## 📋 Compatibility

- **ESP-IDF**: v5.4.3
- **Home Assistant**: All versions supporting MQTT climate entities
- **Hardware**: ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6

## 🤝 Acknowledgments

Special thanks to the Home Assistant community and ESP-IDF contributors for their excellent documentation and tools that made this integration possible.

---

**Full Changelog**: See commit history from `v0.5` to `v0.6`

**Installation Guide**: Refer to the updated README.md for detailed setup instructions</content>
<parameter name="filePath">/home/caffreyfans/work/git/IRbaby_ESP32/RELEASE_NOTES_v0.6.md