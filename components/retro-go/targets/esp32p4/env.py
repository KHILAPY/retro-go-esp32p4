import os

# Set the target chip
IDF_TARGET = "esp32p4"

# Set the firmware format (optional)
# FW_FORMAT = "odroid"

# ESP32-P4 默认禁用网络功能
os.environ["RG_TOOL_NO_NETWORKING"] = "1"

# 直接设置变量（在 exec 后生效）
no_networking = True
