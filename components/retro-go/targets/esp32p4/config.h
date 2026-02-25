#pragma once

#ifdef ESP_PLATFORM
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <driver/spi_master.h>
#include <driver/sdmmc_types.h>
#endif

// Target definition
#define RG_TARGET_NAME             "ESP32-P4"

// ESP32-P4 Performance Configuration
#define RG_TARGET_DUAL_CORE        1
#define RG_TARGET_CPU_FREQ_MHZ     360
#define RG_TARGET_HAS_L2_CACHE     1
#define RG_TARGET_CACHE_LINE_SIZE  64

// Task Core Affinity Configuration
// Core 0: Main application, Input handling, System monitor
// Core 1: Display, SPI, Audio (I/O intensive)
#define RG_CORE_EMULATOR           0
#define RG_CORE_DISPLAY            1
#define RG_CORE_AUDIO              1
#define RG_CORE_INPUT              0
#define RG_CORE_SYSTEM             0

// Emulator build options (controlled by base.cmake)
// RG_EMU_ENABLE_GW, RG_EMU_ENABLE_LYNX, RG_EMU_ENABLE_MSX are defined via compiler options

// Storage
#define RG_STORAGE_ROOT             "/sd"
#define RG_STORAGE_SDMMC_HOST       SDMMC_HOST_SLOT_1
#define RG_STORAGE_SDMMC_SPEED      SDMMC_FREQ_HIGHSPEED
#define RG_GPIO_SDMMC_CLK           GPIO_NUM_43
#define RG_GPIO_SDMMC_CMD           GPIO_NUM_44
#define RG_GPIO_SDMMC_D0            GPIO_NUM_39
#define RG_GPIO_SDMMC_D1            GPIO_NUM_40
#define RG_GPIO_SDMMC_D2            GPIO_NUM_41
#define RG_GPIO_SDMMC_D3            GPIO_NUM_42
#define RG_SDMMC_LDO_CHAN           4

// Audio
#define RG_AUDIO_USE_INT_DAC        0
#define RG_AUDIO_USE_EXT_DAC        0

// Video
// RG_SCREEN_DRIVER: 0=ILI9341/ST7789 (SPI), 1=ST7701 (MIPI DSI)
#define RG_SCREEN_DRIVER            0

#if RG_SCREEN_DRIVER == 0
// ST7789V SPI Display (320x240)
#define RG_SCREEN_HOST              SPI2_HOST
#define RG_SCREEN_SPEED             SPI_MASTER_FREQ_80M
#define RG_SCREEN_BACKLIGHT         1
#define RG_SCREEN_WIDTH             320
#define RG_SCREEN_HEIGHT            240
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
#define RG_SCREEN_INIT()                                                                                         \
    ST7789_CMD(0x11);                                                                                            \
    rg_usleep(120 * 1000);                                                                                       \
    ST7789_CMD(0x36, 0x70);                                                                                      \
    ST7789_CMD(0x3A, 0x05);                                                                                      \
    ST7789_CMD(0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33);                                                              \
    ST7789_CMD(0xB7, 0x71);                                                                                      \
    ST7789_CMD(0xBB, 0x2E);                                                                                      \
    ST7789_CMD(0xC0, 0x2C);                                                                                      \
    ST7789_CMD(0xC2, 0x01);                                                                                      \
    ST7789_CMD(0xC3, 0x10);                                                                                      \
    ST7789_CMD(0xC4, 0x20);                                                                                      \
    ST7789_CMD(0xC6, 0x0F);                                                                                      \
    ST7789_CMD(0xD0, 0xA4, 0xA1);                                                                                \
    ST7789_CMD(0xE0, 0xD0, 0x08, 0x0E, 0x0A, 0x0A, 0x06, 0x38, 0x44, 0x50, 0x29, 0x15, 0x16, 0x33, 0x36);        \
    ST7789_CMD(0xE1, 0xD0, 0x07, 0x0D, 0x09, 0x08, 0x06, 0x33, 0x33, 0x4D, 0x28, 0x16, 0x15, 0x33, 0x35);        \
    ST7789_CMD(0x21);
#elif RG_SCREEN_DRIVER == 1
// ST7701 MIPI DSI Display (480x800 native portrait mode)
#define RG_SCREEN_WIDTH             480
#define RG_SCREEN_HEIGHT            800
#define RG_SCREEN_ROTATE            0
#define RG_SCREEN_VISIBLE_AREA      {0, 0, 0, 0}
#define RG_SCREEN_SAFE_AREA         {0, 0, 0, 0}
#define RG_SCREEN_INIT()            /* ST7701 uses MIPI DSI, init in driver */
#endif


// Input - ADC buttons (resistor ladder on ADC1_CHANNEL_3 / GPIO19)
#define RG_GAMEPAD_ADC_MAP {\
    {RG_KEY_UP,     ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_12, 2500, 2650},\
    {RG_KEY_RIGHT,  ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_12, 200,  400},\
    {RG_KEY_DOWN,   ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_12, 2300, 2450},\
    {RG_KEY_LEFT,   ADC_UNIT_1, ADC_CHANNEL_3, ADC_ATTEN_DB_12, 2080, 2230},\
}
#define RG_GAMEPAD_GPIO_MAP {\
    {RG_KEY_SELECT, .num = GPIO_NUM_8,  .pullup = 1, .level = 0},\
    {RG_KEY_START,  .num = GPIO_NUM_11, .pullup = 1, .level = 0},\
    {RG_KEY_MENU,   .num = GPIO_NUM_14, .pullup = 1, .level = 0},\
    {RG_KEY_OPTION, .num = GPIO_NUM_35, .pullup = 1, .level = 0},\
    {RG_KEY_A,      .num = GPIO_NUM_12, .pullup = 1, .level = 0},\
    {RG_KEY_B,      .num = GPIO_NUM_13, .pullup = 1, .level = 0},\
}

// Battery - disabled
#define RG_BATTERY_DRIVER           0


// Status LED
#define RG_GPIO_LED                 GPIO_NUM_NC

// SPI Display
#define RG_GPIO_LCD_MISO            GPIO_NUM_NC
#define RG_GPIO_LCD_MOSI            GPIO_NUM_29
#define RG_GPIO_LCD_CLK             GPIO_NUM_30
#define RG_GPIO_LCD_CS              GPIO_NUM_31
#define RG_GPIO_LCD_DC              GPIO_NUM_32
#define RG_GPIO_LCD_BCKL            GPIO_NUM_28
#define RG_GPIO_LCD_RST             GPIO_NUM_33

// External I2S DAC
// #define RG_GPIO_SND_I2S_BCK         4
// #define RG_GPIO_SND_I2S_WS          5
// #define RG_GPIO_SND_I2S_DATA        7

// #define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_7
// #define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_15
// #define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_16
// ESP32-P4          MAX98357A
// BCK   ──────>  BCLK
// WS   ──────>  LRCK (LRC)
// DATA   ──────>  DIN
// GND       ──────>  GND
// 3.3V/5V   ──────>  VIN (VCC)
