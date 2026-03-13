// CustomPins.h - Label-set-specific pin definitions for CockpitOS
//
// TFT_IFEI label set — Waveshare ESP32-S3-Touch-LCD-7 (800x480 RGB parallel)
//
// Board:  ESP32-S3-Touch-LCD-7
// LCD:    ST7262 RGB parallel, 800x480, 16-bit RGB565
// Touch:  GT911 capacitive (I2C)
// IO Exp: CH422G (I2C, controls backlight + LCD/touch reset)
//
// This file is automatically included by LabelSetConfig.h if present.
// ============================================================================

#pragma once

// --- Feature Enables --------------------------------------------------------
#define ENABLE_TFT_GAUGES                         1  // 1 = Enable TFT gauge rendering
#define ENABLE_PCA9555                            0  // Not used on this board

// --- I2C Pins (shared: GT911 touch + CH422G IO expander) --------------------
#define SDA_PIN                                   8
#define SCL_PIN                                   9

// --- Waveshare ESP32-S3-Touch-LCD-7 Pin Definitions -------------------------
//
// RGB Parallel Data Bus (directly wired to ESP32-S3 LCD peripheral)
//   Red:   R0=GPIO1, R1=GPIO2, R2=GPIO42, R3=GPIO41, R4=GPIO40
//   Green: G0=GPIO39, G1=GPIO0, G2=GPIO45, G3=GPIO48, G4=GPIO47, G5=GPIO21
//   Blue:  B0=GPIO14, B1=GPIO38, B2=GPIO18, B3=GPIO17, B4=GPIO10
//
// RGB Control Signals
//   HSYNC=GPIO46, VSYNC=GPIO3, PCLK=GPIO7, DE=GPIO5
//
// Touch (GT911)
//   SDA=GPIO8, SCL=GPIO9, INT=GPIO4, RST=CH422G EXIO1
//
// CH422G IO Expander (I2C address-based, NOT register-based)
//   Mode register  @ I2C addr 0x24 (write 0x01 = push-pull output mode)
//   Output register @ I2C addr 0x38 (bit0=EXIO1..bit7=EXIO8)
//   EXIO1 = Touch RST (active low)
//   EXIO2 = LCD Backlight (active high)
//   EXIO3 = LCD Reset (active low, directly handled by LovyanGFX as rst_pin=-1, we do it in init)
//
// These pin constants are consumed by TFT_Display_IFEI.cpp's LGFX device class.
// They are NOT passed through the PIN() macro — this board uses raw ESP32-S3 GPIOs.

// RGB data pins
#define IFEI_TFT_R0                               1
#define IFEI_TFT_R1                               2
#define IFEI_TFT_R2                               42
#define IFEI_TFT_R3                               41
#define IFEI_TFT_R4                               40

#define IFEI_TFT_G0                               39
#define IFEI_TFT_G1                               0
#define IFEI_TFT_G2                               45
#define IFEI_TFT_G3                               48
#define IFEI_TFT_G4                               47
#define IFEI_TFT_G5                               21

#define IFEI_TFT_B0                               14
#define IFEI_TFT_B1                               38
#define IFEI_TFT_B2                               18
#define IFEI_TFT_B3                               17
#define IFEI_TFT_B4                               10

// RGB control signals
#define IFEI_TFT_HSYNC                            46
#define IFEI_TFT_VSYNC                            3
#define IFEI_TFT_PCLK                             7
#define IFEI_TFT_DE                               5

// Touch (GT911)
#define IFEI_TOUCH_SDA                            8
#define IFEI_TOUCH_SCL                            9
#define IFEI_TOUCH_INT                            4

// CH422G I2C addresses (address-based operation, NOT register-based)
#define CH422G_MODE_ADDR                          0x24
#define CH422G_OUTPUT_ADDR                        0x38

// CH422G output bit assignments
#define CH422G_TOUCH_RST_BIT                      0x01  // EXIO1 — GT911 reset (active low)
#define CH422G_BACKLIGHT_BIT                      0x02  // EXIO2 — LCD backlight (active high)
#define CH422G_LCD_RST_BIT                        0x04  // EXIO3 — LCD reset (active low)
