# ESP32-P4 Target

## Overview

ESP32-P4 is a high-performance microcontroller from Espressif with the following features:

- **CPU**: Dual-core RISC-V 32-bit processor, up to 360MHz
- **GPIO**: 55 physical GPIO pins (GPIO0 ~ GPIO54)
- **Architecture**: HP (High Performance) dual-core + LP (Low Power) single-core architecture
- **JTAG**: Built-in USB-JTAG debugging interface

## Configuration

This target configuration includes:

- **Storage**: SD card support via SDMMC (4-bit mode) with internal LDO power
- **Audio**: Disabled (external DAC can be added)
- **Display**: ST7789/ILI9341 SPI display support (320x240 landscape)
- **Input**: ADC buttons (direction keys) + GPIO buttons
- **Battery**: Disabled

## Pinout

### Display (ST7789/ILI9341 - 320x240 Landscape)

| Function | GPIO | Note |
|----------|------|------|
| LCD MISO | GPIO_NUM_NC | Not used |
| LCD MOSI | GPIO_NUM_29 | SPI MOSI |
| LCD CLK | GPIO_NUM_30 | SPI Clock |
| LCD CS | GPIO_NUM_31 | Chip Select |
| LCD DC | GPIO_NUM_32 | Data/Command |
| LCD BCKL | GPIO_NUM_28 | Backlight (PWM) |
| LCD RST | GPIO_NUM_33 | Reset |

**Display Configuration:**
- Resolution: 320x240 (landscape mode)
- Memory Access Control (0x36): 0x70 (MV|MH|BGR for landscape)
- SPI Speed: 80MHz

### SD Card (SDMMC 4-bit mode)

| Function | GPIO |
|----------|------|
| SD CLK | GPIO_NUM_43 |
| SD CMD | GPIO_NUM_44 |
| SD D0 | GPIO_NUM_39 |
| SD D1 | GPIO_NUM_40 |
| SD D2 | GPIO_NUM_41 |
| SD D3 | GPIO_NUM_42 |

**Power Configuration:**
- LDO Channel: 4 (internal LDO for SD card power)
- Speed: SDMMC_FREQ_HIGHSPEED

### Input

按键配置请参考 `config.h` 文件中的 `RG_GAMEPAD_ADC_MAP` 和 `RG_GAMEPAD_GPIO_MAP` 定义。

### Status LED

- **LED**: GPIO_NUM_NC (Not configured)

## Build Commands

To build for ESP32-P4:

```bash
# Set ESP-IDF path and build
set IDF_PATH=E:\Espressif\frameworks\esp-idf-v5.5.2
python rg_tool.py build --target esp32p4
```

To build firmware image:

```bash
python rg_tool.py build-img --target esp32p4
```

To flash the firmware:

```bash
# Flash using esptool
python E:\Espressif\frameworks\esp-idf-v5.5.2\components\esptool_py\esptool\esptool.py --chip esp32p4 --port COM3 --baud 1152000 write_flash 0x0 retro-go_unknown_esp32p4.img

# Or flash individual apps
python E:\Espressif\frameworks\esp-idf-v5.5.2\components\esptool_py\esptool\esptool.py --chip esp32p4 --port COM3 --baud 1152000 write_flash 0x10000 launcher\build\launcher.bin
```

## Known Issues

1. **GPIO_NUM_NC handling**: GPIO pins set to GPIO_NUM_NC are properly skipped during initialization
2. **LEDC**: ESP32-P4 only supports LEDC_LOW_SPEED_MODE
3. **SDMMC**: Uses internal LDO channel 4 for power supply

## Code Modifications for ESP32-P4

### rg_storage.c

1. **SDMMC 4-bit mode support**:
   - Auto-detect 4-bit mode when D1, D2, D3 pins are defined
   - Configure all data pins (D0, D1, D2, D3)
   - Enable internal pull-up resistors

2. **LDO power control**:
   - Added `#include <sd_pwr_ctrl_by_on_chip_ldo.h>`
   - Initialize LDO channel 4 for SD card power
   - Code:
   ```c
   #if CONFIG_IDF_TARGET_ESP32P4 && defined(RG_SDMMC_LDO_CHAN)
       sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;
       sd_pwr_ctrl_ldo_config_t ldo_config = {
           .ldo_chan_id = RG_SDMMC_LDO_CHAN,
       };
       esp_err_t ldo_ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
       if (ldo_ret != ESP_OK) {
           RG_LOGE("Failed to create LDO power control for SDMMC (%s)", esp_err_to_name(ldo_ret));
       } else {
           host_config.pwr_ctrl_handle = pwr_ctrl_handle;
       }
   #endif
   ```

### rg_input.c

1. **GPIO_NUM_NC check in initialization**:
   ```c
   if (mapping->num == GPIO_NUM_NC)
       continue;
   ```

2. **GPIO_NUM_NC check in read function**:
   ```c
   if (mapping->num == GPIO_NUM_NC)
       continue;
   ```

### rg_system.c

- Added GPIO_NUM_NC check for LED GPIO:
  ```c
  #if defined(RG_GPIO_LED) && RG_GPIO_LED != GPIO_NUM_NC
      gpio_set_direction(RG_GPIO_LED, GPIO_MODE_OUTPUT);
      gpio_set_level(RG_GPIO_LED, 0);
  #endif
  ```

### config.h

按键配置请直接修改 `components/retro-go/targets/esp32p4/config.h` 文件中的以下定义：

- `RG_GAMEPAD_ADC_MAP` - ADC 按键映射
- `RG_GAMEPAD_GPIO_MAP` - GPIO 按键映射
