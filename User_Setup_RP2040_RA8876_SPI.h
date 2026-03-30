//                         USER SETUP — RP2040 + RA8876 SPI
// ============================================================
//  Board:   Raspberry Pi Pico (RP2040)
//  Display: EastRising ER-TFTM101-1  1024×600 TFT
//  Driver:  Raydium RA8876
//  Bus:     4-wire hardware SPI (SPI0)
//
//  NOTE: The RA8876 does NOT have a DC (Data/Command) pin.
//  Register select is encoded as a prefix byte in the SPI frame:
//    0x00 = address/command,  0x80 = data write,
//    0x40 = status read,      0xC0 = data read.
//  Therefore TFT_DC must NOT be defined here.
//
//  Additional patches to TFT_eSPI_RA8876.cpp are required for full
//  graphics operation.  See the comment block at the top of
//  TFT_Drivers/RA8876_Defines.h for the exact changes needed.
// ============================================================

#define USER_SETUP_ID 200   // Unique ID — change if conflicts with another setup

// ============================================================
//  Section 1 — Driver selection
// ============================================================

#define RA8876_DRIVER         // EastRising ER-TFTM101-1  1024 x 600

// ============================================================
//  Section 2 — SPI pin assignments for Raspberry Pi Pico
//
//  RP2040 hardware SPI1 pin options:
//    MISO : GPIO  8  (or 12, 24, 28)
//    CS   : GPIO  9  (or 13, 25, 29)  — software-controlled by library
//    SCK  : GPIO 10  (or 14, 26)
//    MOSI : GPIO 11  (or 15, 27)
//    RST  : any GPIO
//
//  Physical wiring (Pico GP numbering):
//    Display pin  1,2  GND  → Pico GND
//    Display pin  3,4  VCC  → Pico VBUS (5 V) or 3V3 depending on module
//    Display pin  5    CS   → Pico GP 9   (TFT_CS)
//    Display pin  6    MISO → Pico GP 8   (TFT_MISO)
//    Display pin  7    MOSI → Pico GP 11  (TFT_MOSI)
//    Display pin  8    SCK  → Pico GP 10  (TFT_SCLK)
//    Display pin  11   RES  → Pico GP 14  (TFT_RST) — also connect NO button to GND here
//    (no DC / RS pin required)
// ============================================================

#define TFT_MISO  8    // SPI1 RX  — MISO from display
#define TFT_MOSI  11   // SPI1 TX  — MOSI to display
#define TFT_SCLK  10   // SPI1 SCK — clock
#define TFT_CS    9    // Chip-select (active low, software controlled)
//      TFT_DC        — NOT defined: RA8876 uses prefix-byte protocol, no DC pin
#define TFT_RST   14   // Hardware reset (active low); also connect NO button to GND for manual reset

// Optional backlight control (leave commented out if backlight is wired to 3V3/5V)
// #define TFT_BL              22   // Backlight GPIO
// #define TFT_BACKLIGHT_ON  HIGH   // HIGH = backlight on

// ============================================================
//  Section 3 — SPI port
//
//  TFT_MOSI=11 and TFT_SCLK=10 are SPI1 hardware pins on the Pico.
//  TFT_SPI_PORT=1 selects the spi1 peripheral to match.
// ============================================================

#define TFT_SPI_PORT  1     // Use RP2040 spi1

// ============================================================
//  Section 4 — SPI frequency
//
//  The RA8876 supports up to 20 MHz on its 4-wire SPI port.
//  Use 10 MHz initially; increase to 20 MHz once wiring is
//  confirmed good (short, well-decoupled connections needed).
// ============================================================

#define SPI_FREQUENCY    20000000    // 20 MHz — maximum per RA8876 spec
#define SPI_READ_FREQUENCY 5000000  // Slower for reliable register reads

// ============================================================
//  Section 5 — Fonts
//  Comment out any you don't need to save FLASH.
// ============================================================

#define LOAD_GLCD    // Font 1: Adafruit 8-px  (~1820 bytes)
#define LOAD_FONT2   // Font 2: 16-px           (~3534 bytes)
#define LOAD_FONT4   // Font 4: 26-px           (~5848 bytes)
#define LOAD_FONT6   // Font 6: 48-px digits    (~2666 bytes)
#define LOAD_FONT7   // Font 7: 7-segment 48-px (~2438 bytes)
#define LOAD_FONT8   // Font 8: 75-px digits    (~3256 bytes)
#define LOAD_GFXFF   // GFX free fonts FF1-FF48

#define SMOOTH_FONT  // Anti-aliased font support (requires LittleFS on Pico)

// ============================================================
//  Section 6 — Miscellaneous
// ============================================================

#define DISABLE_ALL_LIBRARY_WARNINGS  // Suppress TFT_eSPI_RA8876 compile warnings

// Touch screen — the ER-TFTM101-1 uses a GT9271 capacitive touch
// controller on I2C (address 0x5D), not the TFT_eSPI_RA8876 XPT2046 SPI
// touch driver.  Use the bb_captouch library for the GT9271 instead.
// #define TOUCH_CS  -1

// ============================================================
//  Quick-start wiring table
// ============================================================
//
//  ┌──────────────────┬────────────┬─────────────────────────────────────────┐
//  │  Display signal  │  Pico GPIO │  Notes                                  │
//  ├──────────────────┼────────────┼─────────────────────────────────────────┤
//  │  GND  (pin 1,2)  │  GND       │                                         │
//  │  VCC  (pin 3,4)  │  VBUS/5V   │  or 3V3 per module                      │
//  │  CS   (pin 5)    │  GP 9      │  TFT_CS                (SPI1)           │
//  │  MISO (pin 6)    │  GP 8      │  TFT_MISO              (SPI1 RX)        │
//  │  MOSI (pin 7)    │  GP 11     │  TFT_MOSI              (SPI1 TX)        │
//  │  SCK  (pin 8)    │  GP 10     │  TFT_SCLK              (SPI1 SCK)       │
//  │  RES  (pin 11)   │  GP 14     │  TFT_RST; also wire NO button to GND   │
//  │  (no DC pin)     │  —         │  not used for RA8876                    │
//  │  CTPSDA (I2C)    │  GP 12     │  GT9271 SDA  (I2C0, Wire)               │
//  │  CTPSCL (I2C)    │  GP 13     │  GT9271 SCL  (I2C0, Wire)               │
//  │  CTPRST          │  —         │  not software-controlled; NO button to GND │
//  │  CTPINT          │  —         │  not connected; polling needs no INT    │
//  └──────────────────┴────────────┴─────────────────────────────────────────┘
//
//  Touch (GT9271) uses bb_captouch library — see
//  bb_captouch/examples/touch_demo/touch_demo.ino for usage.
//
// ============================================================
