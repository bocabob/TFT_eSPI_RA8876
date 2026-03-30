// RA8876 display controller driver for TFT_eSPI_RA8876
// Target: EastRising ER-TFTM101-1 (1024x600 16-bit colour, RA8876 controller)
// Interface: 4-wire SPI (no DC pin — RA8876 uses a prefix-byte protocol)
//
// RA8876 SPI PROTOCOL (differs from all other TFT_eSPI_RA8876 drivers):
//   Register write:  CS_L  [0x00]  [reg_addr]  CS_H
//                    CS_L  [0x80]  [data_byte]  CS_H
//   Status read:     CS_L  [0x40]  [dummy_read] CS_H
//   Data read:       CS_L  [0xC0]  [dummy_read] CS_H
//
// Because there is NO DC (Data/Command) pin, the TFT_DC pin must NOT be defined
// in User_Setup.  DC_C and DC_D become empty no-ops automatically.
//
// IMPORTANT — additional TFT_eSPI_RA8876.cpp patches required for full operation:
//   1.  writecommand() / writedata() — must send the 0x00 / 0x80 prefix bytes.
//       See the "RA8876 core patches" section at the bottom of this file.
//   2.  setAddrWindow() — RA8876 uses cursor + active-window registers instead
//       of CASET/PASET/RAMWR.  See RA8876_SetAddrWindow patch below.
//   3.  The Init and Rotation files bypass writecommand/writedata entirely and
//       drive SPI directly via the RA8876_CMD / RA8876_DAT macros, so no patch
//       is needed for initialisation to work.

#ifndef RA8876_DEFINES_H
#define RA8876_DEFINES_H

// ============================================================
//  Display geometry (portrait origin = landscape 1024 x 600)
// ============================================================
#define TFT_WIDTH   1024
#define TFT_HEIGHT   600

// ============================================================
//  TFT_eSPI_RA8876 generic command aliases
//  (used by library internals — mapped to RA8876 equivalents)
// ============================================================
#define TFT_NOP      0x00   // No-op  (select dummy reg)
#define TFT_SWRST    0x00   // RA8876 has no SW-reset command; a HW reset is used
#define TFT_INVOFF   0x00   // Not supported
#define TFT_INVON    0x00   // Not supported
#define TFT_DISPOFF  0x12   // DPCR — clear bit 6 to turn display off
#define TFT_DISPON   0x12   // DPCR — set  bit 6 to turn display on
#define TFT_CASET    0x56   // Active Window UL-X  (low byte — 2-reg pair 0x56/0x57)
#define TFT_PASET    0x58   // Active Window UL-Y  (low byte — 2-reg pair 0x58/0x59)
#define TFT_RAMWR    0x04   // MRWDP — Memory Read/Write Data Port (pixel stream)
#define TFT_RAMRD    0x04   // MRWDP read
#define TFT_IDXRD    0x00
#define TFT_MADCTL   0x12   // DPCR controls scan direction / display orientation
#define TFT_MAD_MY   0x08   // DPCR bit 3 — vertical scan direction
#define TFT_MAD_MX   0x10   // DPCR bit 4 — horizontal scan direction
#define TFT_MAD_MV   0x00   // Mirror / rotate — handled via DPCR
#define TFT_MAD_ML   0x00
#define TFT_MAD_BGR  0x00   // RA8876 colour order is configured at panel init
#define TFT_MAD_MH   0x00
#define TFT_MAD_RGB  0x00
#define TFT_MAD_COLOR_ORDER 0x00

// Delay sentinel (used by some init helpers)
#define TFT_INIT_DELAY 0x80

// ============================================================
//  RA8876 register map
// ============================================================

// --- System / PLL ---
#define RA8876_SRN       0x00   // Software Reset / Chip ID (read-only chip ID)
#define RA8876_CCR       0x01   // Core Configuration Register (PLL enable, bus width)
#define RA8876_MACR      0x02   // Memory Access Control Register
#define RA8876_ICR       0x03   // Input Control Register (text/graphics, memory select)
#define RA8876_MRWDP     0x04   // Memory Read/Write Data Port

// PLL clock dividers
#define RA8876_PPLLC1    0x05   // SCLK (scan clock)  PLL control 1
#define RA8876_PPLLC2    0x06   // SCLK PLL control 2 (N multiplier)
#define RA8876_MPLLC1    0x07   // MCLK (SDRAM clock) PLL control 1
#define RA8876_MPLLC2    0x08   // MCLK PLL control 2
#define RA8876_CPLLC1    0x09   // CCLK (core clock)  PLL control 1
#define RA8876_CPLLC2    0x0A   // CCLK PLL control 2

// --- LCD display timing ---
#define RA8876_DPICR     0x10   // Display Image Colour / Pixel-clock configuration
#define RA8876_DPCR      0x12   // Display Panel Configuration (scan dir, DE/HSYNC/VSYNC pol, display on)
#define RA8876_DPIR      0x13   // Display Polarity / Interface Register
#define RA8876_HDWR      0x14   // Horizontal Display Width Register [7:0]   (value = pixels/8 - 1)
#define RA8876_HDWFTR    0x15   // Horizontal Display Width Fine-Tune Register [2:0]
#define RA8876_HNDR      0x16   // Horizontal Non-Display Period Register [7:0] (value = pixels/8 - 1)
#define RA8876_HNDFTR    0x17   // Horizontal Non-Display Fine-Tune Register [3:0]
#define RA8876_HSTR      0x18   // HSYNC Start Position Register [7:0]
#define RA8876_HPWR      0x19   // HSYNC Pulse Width Register [7:0]
#define RA8876_VDHR0     0x1A   // Vertical Display Height Register [7:0]   (value = lines - 1)
#define RA8876_VDHR1     0x1B   // Vertical Display Height Register [12:8]
#define RA8876_VNDR0     0x1C   // Vertical Non-Display Period Register [7:0]
#define RA8876_VNDR1     0x1D   // Vertical Non-Display Period Register [12:8]
#define RA8876_VSTR      0x1E   // VSYNC Start Position Register [7:0]
#define RA8876_VPWR      0x1F   // VSYNC Pulse Width Register [7:0]

// --- Main image / window ---
#define RA8876_MISA0     0x20   // Main Image Start Address [7:0]
#define RA8876_MISA1     0x21   // Main Image Start Address [15:8]
#define RA8876_MISA2     0x22   // Main Image Start Address [23:16]
#define RA8876_MISA3     0x23   // Main Image Start Address [31:24]
#define RA8876_MIW0      0x24   // Main Image Width [7:0]
#define RA8876_MIW1      0x25   // Main Image Width [12:8]
#define RA8876_MWULX0    0x26   // Main Window UL corner X [7:0]
#define RA8876_MWULX1    0x27   // Main Window UL corner X [12:8]
#define RA8876_MWULY0    0x28   // Main Window UL corner Y [7:0]
#define RA8876_MWULY1    0x29   // Main Window UL corner Y [12:8]

// --- Canvas / active window ---
#define RA8876_CVSSA0    0x50   // Canvas Start Address [7:0]
#define RA8876_CVSSA1    0x51   // Canvas Start Address [15:8]
#define RA8876_CVSSA2    0x52   // Canvas Start Address [23:16]
#define RA8876_CVSSA3    0x53   // Canvas Start Address [31:24]
#define RA8876_CVS_IMWTH0 0x54  // Canvas Image Width [7:0]
#define RA8876_CVS_IMWTH1 0x55  // Canvas Image Width [12:8]
#define RA8876_AWUL_X0   0x56   // Active Window UL X [7:0]
#define RA8876_AWUL_X1   0x57   // Active Window UL X [12:8]
#define RA8876_AWUL_Y0   0x58   // Active Window UL Y [7:0]
#define RA8876_AWUL_Y1   0x59   // Active Window UL Y [12:8]
#define RA8876_AW_WTH0   0x5A   // Active Window Width [7:0]
#define RA8876_AW_WTH1   0x5B   // Active Window Width [12:8]
#define RA8876_AW_HT0    0x5C   // Active Window Height [7:0]
#define RA8876_AW_HT1    0x5D   // Active Window Height [12:8]
#define RA8876_CURH0     0x5F   // Memory R/W Cursor Horizontal Position [7:0]
#define RA8876_CURH1     0x60   // Memory R/W Cursor Horizontal Position [12:8]
#define RA8876_CURV0     0x61   // Memory R/W Cursor Vertical Position [7:0]
#define RA8876_CURV1     0x62   // Memory R/W Cursor Vertical Position [12:8]

// Memory port
#define RA8876_MPWCTR    0x5E   // Memory Port W/R Control Register

// --- BTE (Block Transfer Engine) ---
#define RA8876_BTECR0    0x90   // BTE Control Register 0
#define RA8876_BTECR1    0x91   // BTE Control Register 1
#define RA8876_BTECOL    0x9E   // BTE Colour Register (ROP)

// --- PWM / backlight ---
#define RA8876_PSCLR     0x84   // PWM Prescaler (backlight)
#define RA8876_PMUXR     0x85   // PWM Clock MUX Register
#define RA8876_PCFGR     0x86   // PWM Configuration Register
#define RA8876_DZ0       0x88   // PWM0 Dead Zone
#define RA8876_DZ1       0x8A   // PWM1 Dead Zone
#define RA8876_T0CNT0    0x8C   // Timer0 Count Buffer [7:0]
#define RA8876_T0CNT1    0x8D   // Timer0 Count Buffer [15:8]
#define RA8876_T1CNT0    0x8E   // Timer1 Count Buffer [7:0]
#define RA8876_T1CNT1    0x8F   // Timer1 Count Buffer [15:8]

// --- SDRAM control ---
#define RA8876_SDRAR     0xE0   // SDRAM Attribute Register
#define RA8876_SDRMD     0xE1   // SDRAM Mode Register / CAS latency
#define RA8876_SDR_REF0  0xE2   // SDRAM Refresh Interval [7:0]
#define RA8876_SDR_REF1  0xE3   // SDRAM Refresh Interval [15:8]
#define RA8876_SDRCR     0xE4   // SDRAM Control Register (init trigger)

// ============================================================
//  DPCR (reg 0x12) bit definitions
// ============================================================
#define RA8876_DPCR_DISPLAY_ON    0x40  // bit 6
#define RA8876_DPCR_PCLK_FALLING  0x80  // bit 7 — pixel clock edge
#define RA8876_DPCR_VSCAN_BTT     0x08  // bit 3 — vertical scan bottom-to-top
#define RA8876_DPCR_HSCAN_RTL     0x10  // bit 4 — horizontal scan right-to-left

// CCR (reg 0x01) bit definitions
#define RA8876_CCR_PLL_ENABLE     0x80  // bit 7
#define RA8876_CCR_BUS_16BIT      0x01  // bit 0 — host bus width
#define RA8876_CCR_TFT_16BIT      0x10  // bit 4 — TFT colour depth (16 bpp when set)

// MACR (reg 0x02) bit definitions
#define RA8876_MACR_RGB_16B       0x40  // bit 6 — 16-bit colour mode

// ICR (reg 0x03) bit definitions
#define RA8876_ICR_GRAPHIC_MODE   0x00  // bit 2 = 0 — graphic mode
#define RA8876_ICR_TEXT_MODE      0x04  // bit 2 = 1 — text mode
#define RA8876_ICR_MEM_SDRAM      0x00  // bits[1:0] = 00 — canvas points to SDRAM

// MPWCTR (reg 0x5E) bit definitions
#define RA8876_MPWCTR_16BPP       0x01  // bits[1:0] = 01 — 16 bpp
#define RA8876_MPWCTR_XY_ADDR     0x00  // bit 2 = 0 — XY-coordinate addressing

// ============================================================
//  Colour constants (RGB565)
// ============================================================
#define RA8876_BLACK    0x0000
#define RA8876_WHITE    0xFFFF
#define RA8876_RED      0xF800
#define RA8876_GREEN    0x07E0
#define RA8876_BLUE     0x001F
#define RA8876_YELLOW   (RA8876_RED   | RA8876_GREEN)
#define RA8876_CYAN     (RA8876_GREEN | RA8876_BLUE)
#define RA8876_MAGENTA  (RA8876_RED   | RA8876_BLUE)

// ============================================================
//  Direct SPI macros for RA8876 — used by Init and Rotation
//
//  These bypass TFT_eSPI_RA8876's writecommand() / writedata(), which
//  cannot prepend the required 0x00 / 0x80 prefix bytes without
//  patching TFT_eSPI_RA8876.cpp.
//
//  Prerequisites (all defined by TFT_eSPI_RP2040.h before this
//  file is included):
//    CS_L, CS_H         — assert / deassert chip-select
//    tft_Write_8(b)     — send one byte over SPI (waits for idle,
//                         switches SPI to 8-bit, sends, switches back)
//    SPI_BUSY_CHECK     — wait for SPI FIFO to drain + flush RX
// ============================================================

// Write a register address byte (command phase)
#define RA8876_CMD(c) { \
  SPI_BUSY_CHECK; CS_H; \
  CS_L; \
  tft_Write_8(0x00); \
  tft_Write_8((uint8_t)(c)); \
  SPI_BUSY_CHECK; CS_H; \
}

// Write a data byte (data phase)
#define RA8876_DAT(d) { \
  SPI_BUSY_CHECK; CS_H; \
  CS_L; \
  tft_Write_8(0x80); \
  tft_Write_8((uint8_t)(d)); \
  SPI_BUSY_CHECK; CS_H; \
}

// Write one register in a single macro (command + data)
#define RA8876_REG(cmd, data) { RA8876_CMD(cmd); RA8876_DAT(data); }

// Read status register (prefix 0x40)
//   uint8_t v = RA8876_STATUS_READ();
#define RA8876_STATUS_READ()  _ra8876_read_status()

// ============================================================
//  Wait-for-ready helpers (used in Init.h)
//  Implemented as inline lambdas in Init.h because we cannot
//  call non-existent member functions from a #included file.
// ============================================================

// Wait until SDRAM is ready (status byte bit 2 = 1)
#define RA8876_WAIT_SDRAM_READY() \
  { uint8_t _s; do { \
    SPI_BUSY_CHECK; CS_H; CS_L; \
    tft_Write_8(0x40); \
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {}; \
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (8-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS); \
    spi_get_hw(SPI_X)->dr = 0x00; \
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {}; \
    _s = (uint8_t)spi_get_hw(SPI_X)->dr; \
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS); \
    SPI_BUSY_CHECK; CS_H; \
  } while ((_s & 0x04) == 0x00); }

// Wait until SPI/core is not busy (status bit 1 = 0)
#define RA8876_WAIT_CORE_IDLE() \
  { uint8_t _s; do { \
    SPI_BUSY_CHECK; CS_H; CS_L; \
    tft_Write_8(0x40); \
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {}; \
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (8-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS); \
    spi_get_hw(SPI_X)->dr = 0x00; \
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {}; \
    _s = (uint8_t)spi_get_hw(SPI_X)->dr; \
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS); \
    SPI_BUSY_CHECK; CS_H; \
  } while ((_s & 0x02) != 0x00); }

// ============================================================
//  Required patches to TFT_eSPI_RA8876.cpp
//  (copy the blocks below into TFT_eSPI_RA8876.cpp at the locations
//   indicated by the existing XYZZY_DRIVER comments)
//
//  --- Patch 1: User_Setup_Select.h driver list (around line 278) ---
//  #elif defined (RA8876_DRIVER)
//       #include "TFT_Drivers/RA8876_Defines.h"
//       #define  TFT_DRIVER 0x8876
//
//  --- Patch 2: init() driver list (around line 759) ---
//  #elif defined (RA8876_DRIVER)
//      #include "TFT_Drivers/RA8876_Init.h"
//
//  --- Patch 3: setRotation() driver list (find the existing rotation includes) ---
//  #elif defined (RA8876_DRIVER)
//      #include "TFT_Drivers/RA8876_Rotation.h"
//
//  --- Patch 4: writecommand() (around line 979) ---
//  Change:
//    #ifndef RM68120_DRIVER
//  To:
//    #if !defined(RM68120_DRIVER) && !defined(RA8876_DRIVER)
//
//  Then add after the closing #endif:
//  #ifdef RA8876_DRIVER
//  void TFT_eSPI_RA8876::writecommand(uint8_t c) {
//    begin_tft_write();
//    SPI_BUSY_CHECK; CS_H;
//    CS_L; tft_Write_8(0x00); tft_Write_8(c); SPI_BUSY_CHECK; CS_H;
//    end_tft_write();
//  }
//  void TFT_eSPI_RA8876::writedata(uint8_t d) {
//    begin_tft_write();
//    SPI_BUSY_CHECK; CS_H;
//    CS_L; tft_Write_8(0x80); tft_Write_8(d); SPI_BUSY_CHECK; CS_H;
//    end_tft_write();
//  }
//  #endif
//
//  --- Patch 5: setAddrWindow() ---
//  Replace the existing setAddrWindow body with the following for RA8876:
//
//  #ifdef RA8876_DRIVER
//  void TFT_eSPI_RA8876::setAddrWindow(int32_t x0, int32_t y0, int32_t w, int32_t h) {
//    int32_t x1 = x0 + w - 1;
//    int32_t y1 = y0 + h - 1;
//    // Active window
//    RA8876_REG(RA8876_AWUL_X0, x0 & 0xFF);
//    RA8876_REG(RA8876_AWUL_X1, (x0 >> 8) & 0x0F);
//    RA8876_REG(RA8876_AWUL_Y0, y0 & 0xFF);
//    RA8876_REG(RA8876_AWUL_Y1, (y0 >> 8) & 0x0F);
//    RA8876_REG(RA8876_AW_WTH0, w & 0xFF);
//    RA8876_REG(RA8876_AW_WTH1, (w >> 8) & 0x0F);
//    RA8876_REG(RA8876_AW_HT0,  h & 0xFF);
//    RA8876_REG(RA8876_AW_HT1,  (h >> 8) & 0x0F);
//    // Set write cursor to top-left of window
//    RA8876_REG(RA8876_CURH0, x0 & 0xFF);
//    RA8876_REG(RA8876_CURH1, (x0 >> 8) & 0x0F);
//    RA8876_REG(RA8876_CURV0, y0 & 0xFF);
//    RA8876_REG(RA8876_CURV1, (y0 >> 8) & 0x0F);
//    // Select MRWDP for subsequent pixel writes
//    RA8876_CMD(RA8876_MRWDP);
//  }
//  #endif
//
// ============================================================

#endif // RA8876_DEFINES_H
