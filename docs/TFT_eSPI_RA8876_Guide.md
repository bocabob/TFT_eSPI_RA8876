# TFT_eSPI_RA8876 — Usage Guide

A guide to the functions and methods of **TFT_eSPI_RA8876**, a renamed fork of Bodmer's TFT_eSPI v2.5.43 with an added RA8876 driver for the EastRising **ER-TFTM101-1** (1024×600, SPI).

> Choosing between this library and `RA8876_RP2040`? See [Library_Comparison.md](Library_Comparison.md).

---

## Contents

1. [Concepts](#1-concepts)
2. [Configuration](#2-configuration)
3. [Initialization](#3-initialization)
4. [Display control and orientation](#4-display-control-and-orientation)
5. [Colors](#5-colors)
6. [Drawing primitives](#6-drawing-primitives)
7. [Anti-aliased (smooth) graphics](#7-anti-aliased-smooth-graphics)
8. [Text and fonts](#8-text-and-fonts)
9. [Images and bitmaps](#9-images-and-bitmaps)
10. [Viewports and origin](#10-viewports-and-origin)
11. [Sprites (off-screen buffers)](#11-sprites-off-screen-buffers)
12. [Buttons](#12-buttons)
13. [Touch](#13-touch)
14. [Low-level and bus access](#14-low-level-and-bus-access)
15. [RA8876-specific notes and limitations](#15-ra8876-specific-notes-and-limitations)

---

## 1. Concepts

| Item | Name in this library |
|------|----------------------|
| Header | `#include <TFT_eSPI_RA8876.h>` |
| Display class | `TFT_eSPI_RA8876` |
| Sprite class | `TFT_eSprite_RA8876` |
| Button class | `TFT_eSPI_RA8876_Button` |
| Per-sketch setup file | `tft_RA8876_setup.h` |

Every public symbol is renamed, so this library can sit beside the original `TFT_eSPI` in `libraries/` without conflicts.

**How drawing works on the RA8876.** TFT_eSPI is a *pixel-streaming* library: every primitive is rasterized on the MCU and pushed over SPI. For the RA8876, the driver sets the controller's **active window** and **memory cursor** registers (the equivalent of CASET/PASET on MIPI panels) and then streams RGB565 pixels into the memory data port. The RA8876's 2D engine, BTE, PIP and multi-page SDRAM are **not** used.

**Coordinates and types.** Coordinates are `int32_t`. Colors are 16-bit RGB565, passed as `uint32_t`. The origin (0,0) is top-left.

---

## 2. Configuration

The library is configured at compile time with `#define`s. In order of preference:

### 2.1 Sketch-folder setup file (Arduino IDE)

Create `tft_RA8876_setup.h` next to your `.ino`. The library includes it automatically with `__has_include`.

```cpp
// tft_RA8876_setup.h
#pragma once
#define USER_SETUP_LOADED          // required: skip User_Setup_Select.h

#define RA8876_DRIVER              // ER-TFTM101-1, 1024 x 600

#define TFT_SPI_PORT  1            // RP2040 spi1
#define TFT_MISO   8
#define TFT_MOSI  11
#define TFT_SCLK  10
#define TFT_CS     9
#define TFT_RST   14
// Do NOT define TFT_DC: the RA8876 has no D/C pin (it uses prefix bytes)

#define SPI_FREQUENCY       20000000
#define SPI_READ_FREQUENCY   5000000

#define LOAD_GLCD    // Font 1  8 px
#define LOAD_FONT2   // Font 2  16 px
#define LOAD_FONT4   // Font 4  26 px
#define LOAD_FONT6   // Font 6  48 px digits
#define LOAD_FONT7   // Font 7  7-segment 48 px
#define LOAD_FONT8   // Font 8  75 px digits
#define LOAD_GFXFF   // Adafruit GFX FreeFonts
#define SMOOTH_FONT  // Anti-aliased .vlw fonts

// #define RA8876_INIT_TIMEOUT_MS 3000  // see panelFound()
```

A ready-made reference is in the library root: `User_Setup_RP2040_RA8876_SPI.h`.

### 2.2 PlatformIO

Put `tft_RA8876_setup.h` in a folder on the include path (`build_flags = -I src/config`), or pass every define as `-D` flags, including `-D USER_SETUP_LOADED`.

### 2.3 Fallback

With none of the above, the library reads `User_Setup_Select.h` inside the library folder (the classic TFT_eSPI method).

### 2.4 Checking the configuration at runtime

```cpp
setup_t s;
tft.getSetup(s);           // fills s with the settings the compiler adopted
tft.verifySetupID(200);    // true if USER_SETUP_ID matches
tft.fontsLoaded();         // bitmask of fonts compiled in
```

---

## 3. Initialization

```cpp
#include <TFT_eSPI_RA8876.h>

TFT_eSPI_RA8876 tft;                 // width/height default to TFT_WIDTH/TFT_HEIGHT (1024/600)

void setup() {
  Serial.begin(115200);
  tft.init();                        // begin() is an alias
  if (!tft.panelFound()) {
    Serial.println("No RA8876 panel answered - continuing headless");
  }
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
}
```

| Method | Description |
|--------|-------------|
| `TFT_eSPI_RA8876(int16_t w = TFT_WIDTH, int16_t h = TFT_HEIGHT)` | Constructor. |
| `void init(uint8_t tc = TAB_COLOUR)` / `void begin(...)` | Initialise the bus, hardware-reset the panel, program the RA8876 PLLs (scan 50 MHz, SDRAM 120 MHz, core 120 MHz), initialise SDRAM and the panel timing. `tc` is only meaningful for ST7735. |
| `bool panelFound()` | **RA8876 only.** `false` if the last `init()` timed out waiting for PLL lock / SDRAM ready (`RA8876_INIT_TIMEOUT_MS`, default 3000 ms). Lets a sketch keep running without a display. |

---

## 4. Display control and orientation

| Method | Description |
|--------|-------------|
| `void setRotation(uint8_t r)` | 0–3. See the RA8876 caveat below. |
| `uint8_t getRotation()` | Current rotation. |
| `int16_t width()`, `int16_t height()` | Current drawable size. |
| `void invertDisplay(bool i)` | Not supported by the RA8876 (`TFT_INVON`/`TFT_INVOFF` map to no-ops). |
| `void fillScreen(uint32_t color)` | Fill the whole screen. |

**RA8876 rotation.** The driver only changes scan direction (DPCR register), so:

| `r` | Result | `width()×height()` |
|-----|--------|-------------------|
| 0 | Landscape | 1024×600 |
| 1 | Landscape, mirrored horizontally | 1024×600 |
| 2 | Landscape rotated 180° | 1024×600 |
| 3 | Landscape, mirrored vertically | 1024×600 |

True 600×1024 portrait is **not** available with this driver.

---

## 5. Colors

Predefined RGB565 constants: `TFT_BLACK`, `TFT_NAVY`, `TFT_DARKGREEN`, `TFT_DARKCYAN`, `TFT_MAROON`, `TFT_PURPLE`, `TFT_OLIVE`, `TFT_LIGHTGREY`, `TFT_DARKGREY`, `TFT_BLUE`, `TFT_GREEN`, `TFT_CYAN`, `TFT_RED`, `TFT_MAGENTA`, `TFT_YELLOW`, `TFT_WHITE`, `TFT_ORANGE`, `TFT_GREENYELLOW`, `TFT_PINK`, `TFT_BROWN`, `TFT_GOLD`, `TFT_SILVER`, `TFT_SKYBLUE`, `TFT_VIOLET`. The driver also defines `RA8876_BLACK`, `RA8876_WHITE`, `RA8876_RED`, `RA8876_GREEN`, `RA8876_BLUE`, `RA8876_YELLOW`, `RA8876_CYAN`, `RA8876_MAGENTA`.

| Method | Description |
|--------|-------------|
| `uint16_t color565(uint8_t r, uint8_t g, uint8_t b)` | 8-bit R,G,B → RGB565. |
| `uint16_t color8to16(uint8_t c332)` / `uint8_t color16to8(uint16_t c)` | RGB332 ↔ RGB565. |
| `uint32_t color16to24(uint16_t c)` / `uint32_t color24to16(uint32_t c)` | RGB565 ↔ RGB888. |
| `uint16_t alphaBlend(uint8_t alpha, uint16_t fg, uint16_t bg)` | Blend; `alpha` 0 = all bg, 255 = all fg. |
| `uint16_t alphaBlend(uint8_t alpha, uint16_t fg, uint16_t bg, uint8_t dither)` | Blend with dither to reduce banding. |
| `uint32_t alphaBlend24(uint8_t alpha, uint32_t fg, uint32_t bg, uint8_t dither = 0)` | 24-bit blend. |

---

## 6. Drawing primitives

All are rendered on the MCU and streamed as pixels.

| Method | Notes |
|--------|-------|
| `drawPixel(x, y, color)` | One pixel (sets a 1×1 window each call — slow for bulk work). |
| `drawLine(xs, ys, xe, ye, color)` | Bresenham line. |
| `drawFastHLine(x, y, w, color)` / `drawFastVLine(x, y, h, color)` | Fast straight lines. |
| `drawRect(x, y, w, h, color)` / `fillRect(x, y, w, h, color)` | Rectangle outline / filled. |
| `drawRoundRect(x, y, w, h, r, color)` / `fillRoundRect(...)` | Rounded corners, single radius. |
| `fillRectHGradient(x, y, w, h, c1, c2)` / `fillRectVGradient(...)` | Linear gradients. |
| `drawCircle(x, y, r, color)` / `fillCircle(x, y, r, color)` | Circles. |
| `drawCircleHelper(...)` / `fillCircleHelper(...)` | Quarter circles (used for rounded shapes). |
| `drawEllipse(x, y, rx, ry, color)` / `fillEllipse(...)` | Ellipses. |
| `drawTriangle(x1,y1, x2,y2, x3,y3, color)` / `fillTriangle(...)` | Triangles. |

```cpp
tft.fillRectVGradient(0, 0, tft.width(), 80, TFT_NAVY, TFT_BLACK);
tft.drawRoundRect(20, 100, 300, 120, 12, TFT_WHITE);
tft.fillCircle(600, 300, 60, TFT_ORANGE);
```

**Performance tip.** A full-screen fill is 1024×600×2 = 1.2 MB over SPI — roughly half a second at 20 MHz. Redraw only regions that change, or compose into a sprite and push it once.

---

## 7. Anti-aliased (smooth) graphics

| Method | Description |
|--------|-------------|
| `uint16_t drawPixel(x, y, color, uint8_t alpha, uint32_t bg = 0x00FFFFFF)` | Blended pixel; returns the blended color. |
| `drawSmoothArc(x, y, r, ir, startAngle, endAngle, fg, bg, bool roundEnds = false)` | AA arc, AA ends. 0° = 6 o'clock, clockwise. |
| `drawArc(x, y, r, ir, startAngle, endAngle, fg, bg, bool smoothArc = true)` | AA sides, square ends — good for gauges updated in segments. |
| `drawSmoothCircle(x, y, r, fg, bg)` | AA outline. |
| `fillSmoothCircle(x, y, r, color, bg = 0x00FFFFFF)` | AA filled circle. |
| `drawSmoothRoundRect(x, y, r, ir, w, h, fg, bg = 0x00FFFFFF, uint8_t quadrants = 0xF)` | AA rounded border. |
| `fillSmoothRoundRect(x, y, w, h, radius, color, bg = 0x00FFFFFF)` | AA filled rounded rect. |
| `drawSpot(ax, ay, r, fg, bg = 0x00FFFFFF)` | Small AA dot. |
| `drawWideLine(ax, ay, bx, by, wd, fg, bg = 0x00FFFFFF)` | AA thick line with round ends. |
| `drawWedgeLine(ax, ay, bx, by, aw, bw, fg, bg = 0x00FFFFFF)` | AA line tapering from `aw` to `bw` (needles). |

> **Always pass `bg_color` on the RA8876.** When it is omitted the library reads the background pixel back from the display, and pixel read-back is not adapted for the RA8876 (see §15).

```cpp
tft.drawArc(512, 300, 150, 130, 30, 330, TFT_DARKGREY, TFT_BLACK);
tft.drawWedgeLine(512, 300, 620, 220, 8, 2, TFT_RED, TFT_BLACK);
```

---

## 8. Text and fonts

`TFT_eSPI_RA8876` derives from `Print`, so `print()`, `println()` and `printf()` work at the cursor.

### 8.1 Font sources

| Font | Enable with | Select with |
|------|-------------|-------------|
| 1 — GLCD 5×7 (8 px) | `LOAD_GLCD` | `setTextFont(1)` |
| 2 — 16 px | `LOAD_FONT2` | `setTextFont(2)` |
| 4 — 26 px | `LOAD_FONT4` | `setTextFont(4)` |
| 6 — 48 px digits | `LOAD_FONT6` | `setTextFont(6)` |
| 7 — 48 px 7-segment | `LOAD_FONT7` | `setTextFont(7)` |
| 8 — 75 px digits | `LOAD_FONT8` | `setTextFont(8)` |
| Adafruit GFX FreeFonts | `LOAD_GFXFF` | `setFreeFont(&FreeSans18pt7b)` |
| Smooth (anti-aliased) `.vlw` | `SMOOTH_FONT` | `loadFont(...)` |

### 8.2 Text methods

| Method | Description |
|--------|-------------|
| `setTextFont(uint8_t font)` | Select numbered font. |
| `setFreeFont(const GFXfont *f = NULL)` | Select a GFX FreeFont; `NULL` returns to font 1. |
| `setTextSize(uint8_t size)` | Integer pixel multiplier (bitmap fonts). |
| `setTextColor(uint16_t fg)` | Transparent background. |
| `setTextColor(uint16_t fg, uint16_t bg, bool bgfill = false)` | Opaque background; `bgfill` fills the box for smooth fonts. |
| `setTextDatum(uint8_t d)` / `getTextDatum()` | Anchor for `drawString`: `TL_DATUM`, `TC_DATUM`, `TR_DATUM`, `ML_DATUM`, `MC_DATUM`, `MR_DATUM`, `BL_DATUM`, `BC_DATUM`, `BR_DATUM`, `L_BASELINE`, `C_BASELINE`, `R_BASELINE`. |
| `setTextPadding(uint16_t w)` / `getTextPadding()` | Blank to this width when redrawing changing values. |
| `setTextWrap(bool wrapX, bool wrapY = false)` | Wrapping for `print()`. |
| `setCursor(x, y)` / `setCursor(x, y, font)` | Position for `print()`. |
| `getCursorX()`, `getCursorY()` | Cursor after printing. |
| `int16_t drawString(const char*/String, x, y [, font])` | Draw at datum; returns width in px. |
| `int16_t drawNumber(long n, x, y [, font])` | Integer. |
| `int16_t drawFloat(float f, uint8_t dp, x, y [, font])` | Float with `dp` decimals. |
| `int16_t drawChar(uint16_t uniCode, x, y [, font])` | One glyph. |
| `drawCentreString(...)`, `drawRightString(...)` | Deprecated — use `setTextDatum()` + `drawString()`. |
| `int16_t textWidth(str [, font])`, `int16_t fontHeight([font])` | Metrics. |
| `setAttribute(UTF8_SWITCH, true)` | Enable UTF-8 decoding; `CP437_SWITCH` for GLCD code page fix. |

```cpp
tft.setTextColor(TFT_WHITE, TFT_BLACK);
tft.setTextDatum(MC_DATUM);
tft.setTextPadding(tft.textWidth("88.8", 7));
tft.drawFloat(temperature, 1, 512, 300, 7);   // updates in place without flicker
```

### 8.3 Smooth fonts

| Method | Description |
|--------|-------------|
| `loadFont(const uint8_t array[])` | Load a `.vlw` stored as a C array in flash. |
| `loadFont(String name, fs::FS &fs)` | Load from a file system (LittleFS, SD). |
| `loadFont(String name, bool flash = true)` | Load from SPIFFS/LittleFS by name. |
| `unloadFont()` | Free the font and return to bitmap fonts. |
| `showFont(uint32_t ms)` | Display every glyph (debug). |

Create `.vlw` files with `Tools/Create_Smooth_Font/Create_font/Create_font.pde` (Processing).

---

## 9. Images and bitmaps

| Method | Description |
|--------|-------------|
| `pushImage(x, y, w, h, const uint16_t *data)` | RGB565 image from flash. |
| `pushImage(x, y, w, h, uint16_t *data)` | RGB565 image from RAM. |
| `pushImage(x, y, w, h, data, uint16_t transparent)` | Skip pixels equal to `transparent`. |
| `pushImage(x, y, w, h, uint8_t *data, bool bpp8 = true, uint16_t *cmap = nullptr)` | 8-bpp (RGB332) or 4-bpp with palette. |
| `pushMaskedImage(x, y, w, h, uint16_t *img, uint8_t *mask)` | 16-bit image with 1-bpp mask. |
| `setSwapBytes(bool)` / `getSwapBytes()` | Correct endianness of image arrays (most converters need `true`). |
| `drawBitmap(x, y, bitmap, w, h, fg [, bg])` | 1-bpp, MSB first. |
| `drawXBitmap(x, y, bitmap, w, h, fg [, bg])` | 1-bpp XBM, LSB first. |
| `setBitmapColor(fg, bg)` | Colors for 1-bpp sprites. |
| `pushRect(x, y, w, h, uint16_t *data)` | Write a block captured by `readRect`. |
| `setPivot(x, y)`, `getPivotX()`, `getPivotY()` | Pivot for rotated sprites. |
| `readRect(...)`, `readRectRGB(...)`, `readPixel(...)` | Read back — **not reliable on RA8876** (§15). |

`Tools/bmp2array4bit/bmp2array4bit.py` converts a BMP to a 4-bpp array.

---

## 10. Viewports and origin

A viewport clips all drawing to a rectangle and (optionally) moves the origin.

| Method | Description |
|--------|-------------|
| `setViewport(x, y, w, h, bool vpDatum = true)` | Clip to the rectangle; with `vpDatum` true, (0,0) becomes its top-left. |
| `resetViewport()` | Back to full screen. |
| `checkViewport(x, y, w, h)` | `true` if any part of the area is visible. |
| `getViewportX/Y/Width/Height()`, `getViewportDatum()` | Query. |
| `frameViewport(color, int32_t w)` | Draw a frame inside (`w>0`) or outside (`w<0`) the viewport. |
| `setOrigin(x, y)`, `getOriginX()`, `getOriginY()` | Shift the origin without clipping. Reset by `setRotation`/viewport calls. |

---

## 11. Sprites (off-screen buffers)

A sprite is an image in MCU RAM that supports the same drawing API, then is pushed in one burst — flicker-free, and much faster than many small writes.

```cpp
TFT_eSprite_RA8876 spr = TFT_eSprite_RA8876(&tft);

spr.setColorDepth(16);            // 1, 4, 8 or 16
spr.createSprite(300, 120);       // 300*120*2 = 72 KB at 16 bpp
spr.fillSprite(TFT_BLACK);
spr.setTextColor(TFT_YELLOW);
spr.drawString("Speed", 10, 10, 4);
spr.pushSprite(362, 240);         // to the screen
spr.deleteSprite();
```

| Method | Description |
|--------|-------------|
| `void* createSprite(w, h, uint8_t frames = 1)` | Allocate; returns `nullptr` on failure. `frames = 2` for 1/8-bpp double buffering. |
| `void deleteSprite()`, `bool created()` | Free / check. |
| `void* setColorDepth(int8_t bpp)`, `int8_t getColorDepth()` | Set **before** `createSprite`. |
| `createPalette(palette, colors = 16)`, `setPaletteColor(i, c)`, `getPaletteColor(i)` | 4-bpp palette. |
| `fillSprite(color)` | Clear. |
| `pushSprite(x, y [, transparent])` | Push to the display. |
| `pushSprite(tx, ty, sx, sy, sw, sh)` | Push a sub-rectangle. |
| `pushToSprite(dst, x, y [, transparent])` | Compose into another sprite. |
| `pushRotated(angle [, transp])` / `pushRotated(dst, angle [, transp])` | Rotate about the pivot. |
| `getRotatedBounds(...)` | Bounding box of a rotated push. |
| `setScrollRect(x, y, w, h, color = TFT_BLACK)`, `scroll(dx, dy = 0)` | Scroll a zone inside the sprite. |
| `setRotation(r)`, `getRotation()` | Sprite-local rotation. |
| `readPixel(x, y)`, `readPixelValue(x, y)` | Read from sprite memory (works — it is RAM). |
| `void* getPointer()`, `frameBuffer(f)` | Raw buffer access. |
| `printToSprite(...)`, `drawGlyph(code)` | Smooth font rendering into the sprite. |

**RAM budget (RP2040, 264 KB):** 16 bpp = 2·w·h bytes; 8 bpp = w·h; 4 bpp = w·h/2; 1 bpp = w·h/8. A full 1024×600 16-bit frame (1.2 MB) does not fit — use strips or tiles.

---

## 12. Buttons

```cpp
TFT_eSPI_RA8876_Button btn;
btn.initButton(&tft, 512, 500, 200, 60, TFT_WHITE, TFT_BLUE, TFT_WHITE, (char*)"OK", 2);
btn.drawButton();

// in loop, with touch coordinates tx, ty:
btn.press(touched && btn.contains(tx, ty));
if (btn.justPressed())  btn.drawButton(true);
if (btn.justReleased()) btn.drawButton(false);
```

| Method | Description |
|--------|-------------|
| `initButton(gfx, x, y, w, h, outline, fill, text, label, size)` | Centre-referenced. |
| `initButtonUL(gfx, x1, y1, w, h, ...)` | Top-left referenced. |
| `setLabelDatum(dx, dy, datum = MC_DATUM)` | Label offset/anchor. |
| `drawButton(bool inverted = false, String long_name = "")` | Render. |
| `contains(x, y)`, `press(bool)`, `isPressed()`, `justPressed()`, `justReleased()` | State. |

---

## 13. Touch

The built-in touch extension (`getTouch`, `getTouchRaw`, `calibrateTouch`, `setTouch`) supports only **XPT2046 resistive** controllers on the shared SPI bus, enabled by defining `TOUCH_CS`.

The ER-TFTM101-1 uses a **GT9271 capacitive** controller on I²C (address `0x5D`). Leave `TOUCH_CS` undefined and use a separate library such as [bb_captouch](https://github.com/bitbank2/bb_captouch), then feed its coordinates to the button class.

---

## 14. Low-level and bus access

| Method | Description |
|--------|-------------|
| `startWrite()` / `endWrite()` | Hold CS low across many calls (faster batches). |
| `setAddrWindow(x, y, w, h)` / `setWindow(xs, ys, xe, ye)` | Set the RA8876 active window + memory cursor, ready for pixel data. |
| `pushBlock(color, len)` | Stream one color `len` times into the window. |
| `pushPixels(const void *data, len)` | Stream pixel data into the window. |
| `pushColor(color [, len])`, `pushColors(...)`, `writeColor(...)` | Legacy equivalents. |
| `writecommand(uint8_t reg)` / `writedata(uint8_t d)` | **RA8876:** register select (0x00 prefix) / data write (0x80 prefix). |
| `readcommand8/16/32(...)` | MIPI-style reads; not meaningful for the RA8876. |
| `initDMA()`, `pushImageDMA(...)`, `pushPixelsDMA(...)`, `dmaBusy()`, `dmaWait()` | DMA transfers — not verified with the RA8876 driver. |
| `getSPIinstance()` | Underlying `SPIClass&`. |

Writing a raw RA8876 register:

```cpp
tft.startWrite();
tft.writecommand(0x12);   // DPCR
tft.writedata(0xC0);      // display on, normal scan
tft.endWrite();
```

---

## 15. RA8876-specific notes and limitations

- **MCU: RP2040 in practice.** The RA8876 init and register macros access RP2040 SPI hardware registers directly, so `RA8876_DRIVER` is tied to the RP2040 processor port.
- **No D/C pin.** Leave `TFT_DC` undefined; the protocol uses prefix bytes (0x00 register, 0x80 data write, 0x40 status read, 0xC0 data read).
- **SPI clock.** 20 MHz write / 5 MHz read is the tested configuration.
- **No hardware acceleration.** Lines, fills, circles and text are rendered on the MCU and streamed pixel-by-pixel; the RA8876's 2D engine, BTE, PIP, graphic cursor, ROM fonts and multi-page SDRAM are unused.
- **Rotation is mirror-only** (§4) — no true portrait.
- **Pixel read-back is not adapted.** `readPixel`, `readRect`, `readRectRGB`, and smooth-graphics calls that omit `bg_color` rely on read paths written for MIPI controllers. Avoid them, or render in a sprite where reads come from RAM.
- **`invertDisplay()` does nothing** on the RA8876.
- **Diagnostics.** `init()` prints a status line to `Serial`.
- **No-panel boot.** If nothing answers, `init()` returns after `RA8876_INIT_TIMEOUT_MS` and `panelFound()` is `false`.
- **Other controllers.** All the original TFT_eSPI drivers (ILI9341, ST7789, ST7796, GC9A01, SSD1963, …) and processors (ESP32, ESP8266, STM32, RP2040) remain available — select a different driver in the setup file.
