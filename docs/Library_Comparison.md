# TFT_eSPI_RA8876 vs RA8876_RP2040 — Which Should I Use?

Both libraries drive the EastRising **ER-TFTM101-1** (1024×600, RAiO RA8876 controller) from a **Raspberry Pi Pico** over SPI. They take opposite approaches:

- **TFT_eSPI_RA8876** treats the RA8876 like any other TFT: the Pico draws every pixel and streams it to the display. You get the full, familiar TFT_eSPI API.
- **RA8876_RP2040** treats the RA8876 as a graphics co-processor: the Pico sends drawing *commands* and the controller draws into its own 16 MB SDRAM. You get the chip's hardware features.

This file is identical in both repositories. The full API guides are:
[TFT_eSPI_RA8876_Guide.md](https://github.com/bocabob/TFT_eSPI_RA8876/blob/main/docs/TFT_eSPI_RA8876_Guide.md) ·
[RA8876_RP2040_Guide.md](https://github.com/bocabob/RA8876_RP2040/blob/master/docs/RA8876_RP2040_Guide.md)

---

## Quick decision

| If you… | Choose |
|---------|--------|
| Already have TFT_eSPI code, or want to share code with other TFT_eSPI displays | **TFT_eSPI_RA8876** |
| Need sprites, anti-aliased arcs/lines, smooth `.vlw` fonts, text datums | **TFT_eSPI_RA8876** |
| Draw mostly small widgets, text and gauges that update in place | **TFT_eSPI_RA8876** |
| Need fast full-screen fills, large shapes or full-screen redraws | **RA8876_RP2040** |
| Want double buffering, multiple screen pages, PIP overlays, hardware cursor | **RA8876_RP2040** |
| Need true portrait (600×1024) | **RA8876_RP2040** |
| Need to read pixels back from the display | **RA8876_RP2040** |
| Are short on Pico RAM | **RA8876_RP2040** |
| Might move to an ESP32/STM32 or a different display later | **TFT_eSPI_RA8876** (other drivers/processors) |

---

## Side-by-side

| | TFT_eSPI_RA8876 | RA8876_RP2040 |
|---|---|---|
| Origin | Fork of Bodmer's TFT_eSPI 2.5.43 + new RA8876 driver | Port of the Teensy RA8876 library (Watson, mjs513, KurtE, MorganS) |
| Header / class | `TFT_eSPI_RA8876.h` / `TFT_eSPI_RA8876` | `RA8876_RP2040.h` / `RA8876_RP2040` |
| Configuration | `tft_RA8876_setup.h` in sketch (compile-time `#define`s) | `RA8876_Config_SPI.h` in sketch + pins to constructor |
| MCU (RA8876 use) | RP2040 (driver uses RP2040 SPI registers) | RP2040 only |
| SPI port | Selectable (`TFT_SPI_PORT`) | SPI1 only |
| SPI clock | 20 MHz write / 5 MHz read | 30 MHz (verified), 47 MHz possible but unstable |
| Rendering model | MCU rasterizes, streams pixels | RA8876 2D engine draws from commands |
| Full-screen fill | ~1.2 MB over SPI (≈0.5 s at 20 MHz) | A few register writes (near-instant) |
| Rotation | 0/2 landscape; 1/3 are **mirrors only** (always 1024×600) | 0–3 with true 600×1024 portrait |
| Pixel read-back | Not adapted for RA8876 | Works (`readPixel`, `readRect`) |
| Off-screen drawing | MCU RAM sprites (`TFT_eSprite_RA8876`) — limited by 264 KB RAM | 10 full SDRAM pages on the controller; `useCanvas()`/`updateScreen()` |
| Block copy / compositing | Sprite → sprite in MCU RAM | Hardware BTE: copy, ROP, chroma-key, alpha blend, pattern fill |
| Overlays | — | 2 hardware PIP windows |
| Hardware cursor | — | 4× 32×32 graphic cursors + text cursor |
| Anti-aliased graphics | Yes (smooth arcs, circles, wide/wedge lines, round rects) | No |
| Gradients | H/V gradient rects | H/V gradient rects |
| Fonts | GLCD, fonts 2/4/6/7/8, Adafruit GFX FreeFonts, smooth `.vlw` (flash/LittleFS/SD) | RA8876 ROM fonts (8×16/12×24/16×32), user 8×16 CGRAM font, ILI9341_t3 AA fonts (large set included), Adafruit GFX fonts (needs Adafruit_GFX) |
| Text positioning | Datums, padding, `drawString/Number/Float` | `setCursor(CENTER, …)`, margins, terminal-style scroll and clear, status line |
| Images | `pushImage` (16/8/4/1-bpp, transparent, masked), `drawBitmap`, DMA functions | `writeRect`, 1/2/4/8-bpp palette writes, BTE uploads, serial-flash DMA via RA8876 |
| UI helpers | Button class | Status line, graphic cursor |
| Touch | XPT2046 only (not the ER-TFTM101-1's GT9271) | FT5206 optional; GT9271 via bb_captouch (example included) |
| Backlight | External pin only | External pin or RA8876 PWM0/PWM1 |
| No-display boot | `init()` times out, `panelFound()` returns `false` | `begin()` returns `false` |
| Examples in repo | None for RA8876 | 13 (graphics, fonts, BTE, PIP, cursor, rotate, scroll, 3D cube, gauges, touch) |
| Coexists with other libs | Yes — all symbols renamed, installs beside stock TFT_eSPI | Yes |

---

## Performance in practice

**TFT_eSPI_RA8876** cost is proportional to the *pixels* you touch. Small updates (a number, a needle, a 200×100 sprite) are fast. Full-screen backgrounds, large fills and screen clears are slow because every pixel crosses a 20 MHz SPI link.

**RA8876_RP2040** cost is proportional to the *number of commands* for shapes and fills, so large geometry is essentially free. Images still cost their size in SPI bytes the first time, but once in SDRAM they can be redrawn, moved or blended by the BTE at no SPI cost.

Rule of thumb: the larger the area you redraw per frame, the more RA8876_RP2040 wins. The more you rely on anti-aliasing, sprites and rich text layout, the more TFT_eSPI_RA8876 wins.

---

## Same task, both libraries

```cpp
// ---- TFT_eSPI_RA8876 ----
#include <TFT_eSPI_RA8876.h>          // pins in tft_RA8876_setup.h
TFT_eSPI_RA8876 tft;

void setup() {
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.fillRoundRect(100, 100, 300, 150, 12, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Hello", 250, 175, 4);
}
```

```cpp
// ---- RA8876_RP2040 ----
#include "RA8876_Config_SPI.h"
#include <SPI.h>
#include <RA8876_RP2040.h>
#include "font_Arial.h"
RA8876_RP2040 tft(RA8876_CS, RA8876_RESET, RA8876_MOSI, RA8876_SCLK, RA8876_MISO);

void setup() {
  tft.begin(30000000);
  tft.fillScreen(BLACK);
  tft.fillRoundRect(100, 100, 300, 150, 12, 12, BLUE);
  tft.setFont(Arial_24);
  tft.setTextColor(WHITE, BLUE);
  tft.setCursor(200, 160);
  tft.print("Hello");
}
```

### API name mapping

| Task | TFT_eSPI_RA8876 | RA8876_RP2040 |
|------|-----------------|---------------|
| Start | `init()` / `begin()` | `begin(spi_hz)` |
| Rounded rect | `fillRoundRect(x,y,w,h,r,c)` | `fillRoundRect(x,y,w,h,xr,yr,c)` |
| Filled circle | `fillCircle(x,y,r,c)` | `fillCircle(x,y,r,c)` / `drawCircleFill(...)` |
| Rect by corners | — | `drawSquare` / `drawSquareFill(x0,y0,x1,y1,c)` |
| Select font | `setTextFont(n)`, `setFreeFont(&f)`, `loadFont(...)` | `setFontSize(0..2)`, `setFont(Arial_14)`, `setFont(&FreeSans12pt7b)` |
| Centered text | `setTextDatum(MC_DATUM); drawString(...)` | `setCursor(CENTER, y); print(...)` |
| Image | `pushImage(x,y,w,h,data)` | `writeRect(x,y,w,h,data)` |
| Off-screen compose | `TFT_eSprite_RA8876` + `pushSprite()` | `useCanvas(true)` … `updateScreen()` |
| Colors | `TFT_RED`, `color565()` | `RED`, `COLOR65K_RED`, `color565()` |

---

## Using both

Both libraries can be installed together. They should **not** drive the same panel in one sketch: each runs its own RA8876 initialisation and assumes it owns the controller's registers.

---

## Summary

- **TFT_eSPI_RA8876** — portable, feature-rich software renderer with the well-known TFT_eSPI API. Best for widget-style UIs with anti-aliased graphics and nice fonts where only small regions change.
- **RA8876_RP2040** — uses the RA8876's own hardware. Best for full-screen graphics, page flipping, overlays, portrait mode and anything that would otherwise push large pixel areas over SPI.
