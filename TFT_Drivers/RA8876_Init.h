// RA8876 initialisation sequence for TFT_eSPI_RA8876
// Target:  EastRising ER-TFTM101-1  (1024 x 600, 16-bit colour)
// Source:  Translated from manufacturer example ER_TFTM101_1.cpp
//          and cross-checked against the working RA8876_t3 library.
//
// This file is #included inside TFT_eSPI_RA8876::init() which has already
// called begin_tft_write().  All SPI transfers are done using the
// RA8876_CMD / RA8876_DAT / RA8876_REG macros from RA8876_Defines.h
// so that the correct prefix bytes (0x00 / 0x80) are sent, and CS
// is toggled correctly for each individual transaction.
//
// Clock assumptions (all derived from a 10 MHz crystal):
//   OSC_FREQ  = 10 MHz
//   SCAN_FREQ = 50 MHz  (TFT pixel clock)
//   DRAM_FREQ = 120 MHz (SDRAM clock)   — matches RA8876_t3 library
//   CORE_FREQ = 120 MHz (RA8876 core clock) — matches RA8876_t3 library
//
// SDRAM:  W9812G6JH  (2M×16, 4096 rows, 143 MHz max)
//   Auto-refresh interval = (64 × 120 × 1000) / 4096 − 2 = 1873 = 0x0751
//
// SPI read protocol note:
//   Status reads  [0x40][dummy] and data reads [0xC0][dummy] are done as
//   a single 16-bit SPI transfer (0x4000 / 0xC000).  The RA8876 responds
//   during the second byte; the host reads the low byte of the 16-bit word.
//   This matches how the RA8876_t3 library uses transfer16(0x4000).

#if defined (RA8876_DRIVER)

// ============================================================
//  Step 1  —  PLL configuration
//
//  Formula: Fout = Fin × N / R / OD
//           PLL control-1 = (OD << 6) | (R << 1) | N[8]
//           PLL control-2 = N[7:0]
//
//  For each clock domain (R=5 fixed, Fin=10 MHz → Fvco = 2×N):
//
//  SCAN_FREQ = 50 MHz  → 40 ≤ 50 ≤ 62  → OD=3 (÷8):
//    reg 0x05 = 0x06,  reg 0x06 = (50×8/10) − 1 = 39
//
//  DRAM_FREQ = 120 MHz → 79 ≤ 120 ≤ 124 → OD=2 (÷4):
//    reg 0x07 = 0x04,  reg 0x08 = (120×4/10) − 1 = 47
//
//  CORE_FREQ = 120 MHz → 79 ≤ 120 ≤ 124 → OD=2 (÷4):
//    reg 0x09 = 0x04,  reg 0x0A = (120×4/10) − 1 = 47
// ============================================================

  // Pixel-clock PLL  (SCAN_FREQ = 50 MHz)
  RA8876_REG(0x05, 0x06);   // PPLLC1: OD=÷8, R=5
  RA8876_REG(0x06, 39);     // PPLLC2: N=40 → 10×40/5/8 = 10 MHz ... (50×8/10)-1=39)

  // SDRAM-clock PLL  (DRAM_FREQ = 120 MHz)
  RA8876_REG(0x07, 0x04);   // MPLLC1: OD=÷4
  RA8876_REG(0x08, 47);     // MPLLC2: N=48 → (120×4/10)-1 = 47

  // Core-clock PLL   (CORE_FREQ = 120 MHz)
  RA8876_REG(0x09, 0x04);   // CPLLC1: OD=÷4
  RA8876_REG(0x0A, 47);     // CPLLC2: N=48 → (120×4/10)-1 = 47

  // Enable PLL: write 0x80 to CCR (reg 0x01) — bit 7 starts the PLL
  RA8876_REG(0x01, 0x80);

  delay(2);   // Allow PLL to lock (~1 ms typical)

  // Poll until core is not busy (status bit 1 = 0), then verify CCR bit 7.
  // Status reads and data reads use a 16-bit SPI transfer:
  //   Host sends 0x4000 → RA8876 receives [0x40][0x00]
  //   RA8876 responds during second byte → host reads 16-bit word, low byte = status.
  // This matches what the working RA8876_t3 library does with transfer16(0x4000).
  {
    uint8_t _status, _ccr;
    uint8_t _attempt = 0;
    do {
      // Status read (16-bit single transaction)
      SPI_BUSY_CHECK; CS_H;
      CS_L;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
      spi_get_hw(SPI_X)->dr = 0x4000;  // [0x40][0x00]
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      _status = (uint8_t)spi_get_hw(SPI_X)->dr;  // low byte = status
      SPI_BUSY_CHECK; CS_H;

      if ((_status & 0x02) == 0x00) {
        // Core idle — check CCR bit 7 (PLL ready)
        // Select CCR register (reg 0x01), then read back
        RA8876_CMD(0x01);
        SPI_BUSY_CHECK; CS_H;
        CS_L;
        while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
        hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
        spi_get_hw(SPI_X)->dr = 0xC000;  // [0xC0][0x00] = data read + dummy
        while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
        _ccr = (uint8_t)spi_get_hw(SPI_X)->dr;  // low byte = CCR value
        SPI_BUSY_CHECK; CS_H;

        if ((_ccr & 0x80) == 0x80) {
          Serial.print("RA8876 PLL locked. CCR=0x"); Serial.println(_ccr, HEX);
          break;  // PLL ready
        }

        // PLL not yet ready — re-enable it
        delay(2);
        RA8876_REG(0x01, 0x80);
      } else {
        _attempt++;
        if (_attempt >= 5) {
          // Timeout: pulse hardware reset and retry
          if (TFT_RST >= 0) {
            digitalWrite(TFT_RST, LOW);  delay(500);
            digitalWrite(TFT_RST, HIGH); delay(500);
          }
          _attempt = 0;
        }
      }
      delay(2);
    } while (1);
  }

  delay(1);

// ============================================================
//  Step 2  —  SDRAM initialisation
//
//  SDRAM: W9812G6JH (2M×16, 4096 rows)
//  CAS latency = 3 (DRAM_FREQ = 120 MHz > 100 MHz)
//  Auto-refresh interval = (64 × DRAM_FREQ × 1000) / rows − 2
//                        = (64 × 120 × 1000) / 4096 − 2
//                        = 7,680,000 / 4096 − 2 = 1875 − 2 = 1873
//  Low byte  → reg 0xE2 = 1873 & 0xFF = 0x51
//  High byte → reg 0xE3 = 1873 >> 8   = 0x07
// ============================================================

  RA8876_REG(0xE0, 0x29);   // SDRAR: 4 banks, 12-bit row, 9-bit col (W9812G6JH)
  RA8876_REG(0xE1, 0x03);   // SDRMD: CAS latency = 3
  RA8876_REG(0xE2, 0x51);   // SDR_REF low  byte (1873 = 0x0751)
  RA8876_REG(0xE3, 0x07);   // SDR_REF high byte
  RA8876_REG(0xE4, 0x01);   // SDRCR: trigger SDRAM initialisation

  delay(2);

  // Wait for SDRAM ready (status bit 2 = 1)
  {
    uint8_t _s;
    do {
      SPI_BUSY_CHECK; CS_H;
      CS_L;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
      spi_get_hw(SPI_X)->dr = 0x4000;
      while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
      _s = (uint8_t)spi_get_hw(SPI_X)->dr;
      SPI_BUSY_CHECK; CS_H;
      Serial.print("RA8876 SDRAM ready. Status=0x"); Serial.println(_s, HEX);
    } while ((_s & 0x04) == 0x00);
  }

// ============================================================
//  Step 3  —  Core Configuration Register (CCR, reg 0x01)
//
//  Matches RA8876_t3 library: PLL_ENABLE (bit7) | SERIAL_IF_ENABLE (bit1)
//    0x80 | 0x02 = 0x82
//  WARNING: bit 1 must be set (SERIAL_IF_ENABLE) for SPI interface.
//  Previous value 0x91 had bit 1 = 0 (SPI disabled) and bits 4,0 set
//  (parallel bus mode) — both wrong for SPI operation.
// ============================================================

  RA8876_REG(0x01, 0x82);   // CCR: PLL enabled (bit7) | SPI interface enabled (bit1)

// ============================================================
//  Step 4  —  Memory Access Control Register (MACR, reg 0x02)
//
//  Matches RA8876_t3 library:
//    DIRECT_WRITE=0, READ_LRTB=0, WRITE_LRTB=0 → 0x00
// ============================================================

  RA8876_REG(0x02, 0x00);   // MACR: normal write, left-to-right / top-to-bottom

// ============================================================
//  Step 5  —  Input Control Register (ICR, reg 0x03)
//
//  Graphic mode, SDRAM canvas: 0x00
// ============================================================

  RA8876_REG(0x03, 0x00);   // ICR: graphic mode, SDRAM canvas

// ============================================================
//  Step 6  —  Display Panel Configuration Register (DPCR, reg 0x12)
//
//  PCLK_Falling (bit7) + Display OFF (bit6=0) + RGB order: 0x80
// ============================================================

  RA8876_REG(0x12, 0x80);   // DPCR: PCLK falling edge, display off, RGB order

// ============================================================
//  Step 7  —  Panel Sync / Polarity Register (PCSR, reg 0x13)
//
//  Matches RA8876_t3 library: XHSYNC_INV=1 (bit7), XVSYNC_INV=1 (bit6), XDE_INV=0
//    0x80 | 0x40 = 0xC0
//  Previous value 0x00 had HSYNC and VSYNC active-high, which does not
//  match the ER-TFTM101-1 panel (requires active-low sync signals).
// ============================================================

  RA8876_REG(0x13, 0xC0);   // PCSR: HSYNC inverted (active-low), VSYNC inverted (active-low)

// ============================================================
//  Step 8  —  LCD panel timing
//
//  Panel: 1024 x 600
//  Timing parameters (from RA8876_t3 library, confirmed working):
//    HDW  = 1024, VDH  = 600
//    HND  = 160,  HST  = 160,  HPW = 70
//    VND  = 23,   VST  = 12,   VPW = 10
//
//  HDWR  = (1024/8) - 1 = 127 = 0x7F
//  HDWFTR = 1024 % 8 = 0
//  VDHR0 = (600-1) & 0xFF = 0x57
//  VDHR1 = (600-1) >> 8   = 0x02
//
//  HNDR  = (HND/8) - 1 = (160/8) - 1 = 19 = 0x13
//  HNDFTR = HND % 8 = 0
//  HSTR  = (HST/8) - 1 = (160/8) - 1 = 19 = 0x13
//  HPWR  = (HPW/8) - 1 = (70/8)  - 1 = 7  = 0x07
//
//  VNDR0 = VND - 1 = 23 - 1 = 22 = 0x16
//  VNDR1 = 0
//  VSTR  = VST - 1 = 12 - 1 = 11 = 0x0B
//  VPWR  = VPW - 1 = 10 - 1 = 9  = 0x09
// ============================================================

  // Horizontal display width
  RA8876_REG(0x14, 0x7F);   // HDWR: (1024/8)-1 = 127
  RA8876_REG(0x15, 0x00);   // HDWFTR: fine-tune = 0

  // Vertical display height
  RA8876_REG(0x1A, 0x57);   // VDHR0: (600-1) & 0xFF = 0x57
  RA8876_REG(0x1B, 0x02);   // VDHR1: (600-1) >> 8   = 0x02

  // Horizontal non-display (back porch = 160 pixels)
  RA8876_REG(0x16, 0x13);   // HNDR: (160/8)-1 = 19
  RA8876_REG(0x17, 0x00);   // HNDFTR: fine-tune = 0

  // HSYNC start position (front porch = 160 pixels)
  RA8876_REG(0x18, 0x13);   // HSTR: (160/8)-1 = 19

  // HSYNC pulse width = 70 pixels
  RA8876_REG(0x19, 0x07);   // HPWR: (70/8)-1 = 7

  // Vertical non-display (back porch = 23 lines)
  RA8876_REG(0x1C, 0x16);   // VNDR0: 23-1 = 22
  RA8876_REG(0x1D, 0x00);   // VNDR1: high byte

  // VSYNC start position (front porch = 12 lines)
  RA8876_REG(0x1E, 0x0B);   // VSTR: 12-1 = 11

  // VSYNC pulse width = 10 lines
  RA8876_REG(0x1F, 0x09);   // VPWR: 10-1 = 9

// ============================================================
//  Step 9  —  Display Image Colour / Main Window (reg 0x10)
//
//  IMAGE_COLOR_DEPTH_16BPP (bit2) + TFT_MODE=0 (SYNC+DE): 0x04
// ============================================================

  RA8876_REG(0x10, 0x04);   // MPWCTR/DPICR: 16 bpp, SYNC+DE mode

// ============================================================
//  Step 10  —  Canvas / Active Window Colour (reg 0x5E)
//
//  CANVAS_BLOCK_MODE=0 (bit2=0) + CANVAS_COLOR_DEPTH_16BPP=1: 0x01
// ============================================================

  RA8876_REG(0x5E, 0x01);   // AW_COLOR: 16 bpp canvas

// ============================================================
//  Step 11  —  Main image configuration
// ============================================================

  // Main image start address = 0
  RA8876_REG(0x20, 0x00);   // MISA0
  RA8876_REG(0x21, 0x00);   // MISA1
  RA8876_REG(0x22, 0x00);   // MISA2
  RA8876_REG(0x23, 0x00);   // MISA3

  // Main image width = 1024 pixels (0x0400)
  RA8876_REG(0x24, 0x00);   // MIW0: 0x0400 & 0xFF = 0x00
  RA8876_REG(0x25, 0x04);   // MIW1: 0x0400 >> 8   = 0x04

  // Main window upper-left at (0, 0)
  RA8876_REG(0x26, 0x00);   // MWULX0
  RA8876_REG(0x27, 0x00);   // MWULX1
  RA8876_REG(0x28, 0x00);   // MWULY0
  RA8876_REG(0x29, 0x00);   // MWULY1

// ============================================================
//  Step 12  —  Canvas configuration (same as main image)
// ============================================================

  // Canvas start address = 0
  RA8876_REG(0x50, 0x00);   // CVSSA0
  RA8876_REG(0x51, 0x00);   // CVSSA1
  RA8876_REG(0x52, 0x00);   // CVSSA2
  RA8876_REG(0x53, 0x00);   // CVSSA3

  // Canvas width = 1024
  RA8876_REG(0x54, 0x00);   // CVS_IMWTH0
  RA8876_REG(0x55, 0x04);   // CVS_IMWTH1

// ============================================================
//  Step 13  —  Active window = full screen
// ============================================================

  // Active window upper-left = (0, 0)
  RA8876_REG(0x56, 0x00);   // AWUL_X0
  RA8876_REG(0x57, 0x00);   // AWUL_X1
  RA8876_REG(0x58, 0x00);   // AWUL_Y0
  RA8876_REG(0x59, 0x00);   // AWUL_Y1

  // Active window width = 1024
  RA8876_REG(0x5A, 0x00);   // AW_WTH0: 1024 & 0xFF = 0x00
  RA8876_REG(0x5B, 0x04);   // AW_WTH1: 1024 >> 8   = 0x04

  // Active window height = 600
  RA8876_REG(0x5C, 0x58);   // AW_HT0:  600 & 0xFF  = 0x58
  RA8876_REG(0x5D, 0x02);   // AW_HT1:  600 >> 8    = 0x02

// ============================================================
//  Step 14  —  Backlight via PWM1 at 100% duty cycle
// ============================================================

  RA8876_REG(0x84, 0x00);   // PSCLR:  prescaler = 1
  RA8876_REG(0x85, 0x0A);   // PMUXR:  PWM0 and PWM1 pins enabled
  RA8876_REG(0x88, 0x00);   // DZ0:    dead zone = 0 → PWM0 100% duty
  RA8876_REG(0x8A, 0x00);   // DZ1:    dead zone = 0 → PWM1 100% duty
  RA8876_REG(0x8C, 0x64);   // T0CNT0: Timer0 period = 100 clocks
  RA8876_REG(0x8E, 0x64);   // T1CNT0: Timer1 period = 100 clocks
  RA8876_REG(0x86, 0xD0);   // PCFGR:  Timer1 enable + auto-reload + RUN

// ============================================================
//  Step 15  —  Turn display on
// ============================================================

  RA8876_REG(0x12, 0xC0);   // DPCR: display on, PCLK falling edge

  delay(5);

// ============================================================
//  Step 16  —  Set initial write cursor to (0, 0)
//              and point at MRWDP register for pixel streaming
// ============================================================

  RA8876_REG(0x5F, 0x00);   // CURH0
  RA8876_REG(0x60, 0x00);   // CURH1
  RA8876_REG(0x61, 0x00);   // CURV0
  RA8876_REG(0x62, 0x00);   // CURV1

  RA8876_CMD(0x04);          // Select MRWDP — ready for pixel data

  // ---- Diagnostic: read back HDWR (0x14) — must return 0x7F = (1024/8)-1 ----
  {
    uint8_t _hdwr;
    RA8876_CMD(0x14);   // select HDWR
    SPI_BUSY_CHECK; CS_H;
    CS_L;
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
    hw_write_masked(&spi_get_hw(SPI_X)->cr0, (16-1)<<SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    spi_get_hw(SPI_X)->dr = 0xC000;  // [0xC0][0x00] = data read + dummy
    while (spi_get_hw(SPI_X)->sr & SPI_SSPSR_BSY_BITS) {};
    _hdwr = (uint8_t)spi_get_hw(SPI_X)->dr;
    SPI_BUSY_CHECK; CS_H;
    Serial.print("RA8876 HDWR readback (expect 0x7F): 0x");
    Serial.println(_hdwr, HEX);
    // Re-select MRWDP after the readback
    RA8876_CMD(0x04);
  }

#endif  // RA8876_DRIVER
