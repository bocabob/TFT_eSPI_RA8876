# TFT_eSPI_RA8876

A fork of [Bodmer's TFT_eSPI library](https://github.com/Bodmer/TFT_eSPI) modified to support the **ER-TFTM101-1** (1024×600, RA8876 SPI driver) and restructured to coexist alongside the standard TFT_eSPI library without conflict.

## Differences from TFT_eSPI

| | TFT_eSPI | TFT_eSPI_RA8876 |
|---|---|---|
| Library folder | `TFT_eSPI` | `TFT_eSPI_RA8876` |
| Main header | `TFT_eSPI.h` | `TFT_eSPI_RA8876.h` |
| Main class | `TFT_eSPI` | `TFT_eSPI_RA8876` |
| Sprite class | `TFT_eSprite` | `TFT_eSprite_RA8876` |
| Button class | `TFT_eSPI_Button` | `TFT_eSPI_RA8876_Button` |
| Sketch setup file | `tft_setup.h` | `tft_RA8876_setup.h` |

Because every public symbol is renamed, both libraries can be installed in the Arduino `libraries/` folder simultaneously and included in the same project without symbol conflicts.

---

## Installation

Copy the `TFT_eSPI_RA8876` folder into your Arduino `libraries/` directory (typically `~/Documents/Arduino/libraries/`), then restart the Arduino IDE.

In your sketch:
```cpp
#include <TFT_eSPI_RA8876.h>

TFT_eSPI_RA8876 tft;
TFT_eSprite_RA8876 spr = TFT_eSprite_RA8876(&tft);
```

---

## Hardware Configuration (per-project setup)

The library is configured through a hardware setup file that defines the display driver, SPI pins, and frequencies. **You should not need to edit any file inside the library folder.**

### Method 1 — Sketch-folder file (Arduino IDE, recommended)

Create a file named **`tft_RA8876_setup.h`** in your sketch folder. The library automatically detects and uses it, bypassing the library's internal `User_Setup_Select.h`.

**Example `tft_RA8876_setup.h` for the ER-TFTM101-1 on RP2040:**
```cpp
#pragma once
#define USER_SETUP_LOADED       // tells the library this file is the config

#define RA8876_DRIVER

// SPI pins (adjust to match your wiring)
#define TFT_MOSI  19
#define TFT_MISO  16
#define TFT_SCLK  18
#define TFT_CS    17
#define TFT_RST   20

#define SPI_FREQUENCY   20000000
#define SPI_READ_FREQUENCY  5000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
```

The key line is `#define USER_SETUP_LOADED` — once that is defined, the library skips `User_Setup_Select.h` entirely.

### Method 2 — Include-path file (PlatformIO)

Place `tft_RA8876_setup.h` anywhere in your project (e.g. `src/config/`) and add its directory to `build_flags` in `platformio.ini`:

```ini
[env:my_board]
platform = raspberrypi
board = rpipico
framework = arduino
lib_deps =
    TFT_eSPI_RA8876
build_flags =
    -I src/config
```

### Method 3 — PlatformIO `build_flags` defines

Alternatively, define all configuration macros directly in `platformio.ini`:

```ini
build_flags =
    -D USER_SETUP_LOADED
    -D RA8876_DRIVER
    -D TFT_MOSI=19
    -D TFT_MISO=16
    -D TFT_SCLK=18
    -D TFT_CS=17
    -D SPI_FREQUENCY=20000000
```

### Fallback — Edit the library directly

If none of the above methods are used, the library falls back to `User_Setup_Select.h` in the library folder. Edit that file to uncomment the desired setup, or add your own setup file to the `User_Setups/` folder and reference it there. This is the original TFT_eSPI approach and works, but means editing library files.

---

## Supported Processors

Optimised drivers have been tested with the following processors:

* RP2040 (Raspberry Pi Pico)
* ESP32, ESP32-S2, ESP32-C3, ESP32-S3
* ESP8266
* STM32F1xx, STM32F2xx, STM32F4xx, STM32F767

| Processor | 4 wire SPI | 8-bit parallel | 16-bit parallel | DMA support |
|-----------|:---:|:---:|:---:|:---:|
| RP2040    | Yes | Yes | Yes | Yes (all) |
| ESP32     | Yes | Yes | No  | Yes (SPI only) |
| ESP32 C3  | Yes | No  | No  | No |
| ESP32 S2  | Yes | No  | No  | No |
| ESP32 S3  | Yes | Yes | No  | Yes (SPI only) |
| ESP8266   | Yes | No  | No  | No |
| STM32Fxxx | Yes | Yes | No  | Yes (SPI only) |
| Other     | Yes | No  | No  | No |

---

## Supported Display Controllers

* RA8876 *(primary target — ER-TFTM101-1, 1024×600)*
* GC9A01
* ILI9163, ILI9225, ILI9341, ILI9342, ILI9481, ILI9486, ILI9488
* HX8357B, HX8357C, HX8357D
* R61581
* RM68120, RM68140
* S6D02A1
* SSD1351
* SSD1963 (parallel interface only)
* ST7735, ST7789, ST7796

---

## Sprites

A `TFT_eSprite_RA8876` is an off-screen graphics buffer held in RAM. Draw into it exactly as you would the display, then push it to the screen in one operation for flicker-free updates.

```cpp
TFT_eSprite_RA8876 spr = TFT_eSprite_RA8876(&tft);
spr.createSprite(120, 60);
spr.fillSprite(TFT_BLACK);
spr.drawString("Hello", 10, 20, 2);
spr.pushSprite(x, y);
```

RAM needed: 2 × width × height bytes (16-bit colour), or width × height bytes (8-bit colour).
On ESP32 with PSRAM, large full-screen sprites are possible.

---

## Touch Controller

XPT2046 touch screen support is built in for SPI displays. The SPI bus is shared with the TFT; only an additional chip-select pin is required. Configure `TFT_TOUCH_CS` and `SPI_TOUCH_FREQUENCY` in your setup file.

---

## Fonts

The library includes built-in proportional fonts (sizes 2, 4, 6, 7, 8) and the full Adafruit GFX free-font collection. Fonts are enabled in your setup file:

```cpp
#define LOAD_GLCD   // Font 1 — small
#define LOAD_FONT2  // Font 2 — 16px
#define LOAD_FONT4  // Font 4 — 26px
#define LOAD_FONT6  // Font 6 — 48px digits
#define LOAD_FONT7  // Font 7 — 7-segment style
#define LOAD_FONT8  // Font 8 — 75px digits
#define LOAD_GFXFF  // FreeFonts (Adafruit GFX compatible)
#define SMOOTH_FONT // Anti-aliased vlw fonts from SPIFFS/LittleFS
```

Anti-aliased (smooth) fonts in `.vlw` format can be generated with the Processing sketch in the `Tools/` folder and uploaded to SPIFFS, LittleFS, or an SD card.

---

## Upstream

This library is based on [TFT_eSPI v2.5.43 by Bodmer](https://github.com/Bodmer/TFT_eSPI). The RA8876 driver support and ER-TFTM101-1 setup files were added on top of that base. All class and symbol names have been prefixed/suffixed with `_RA8876` to allow side-by-side installation with the original library.
