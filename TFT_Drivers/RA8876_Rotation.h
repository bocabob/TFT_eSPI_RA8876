// RA8876 rotation for TFT_eSPI_RA8876
// This file is #included inside TFT_eSPI_RA8876::setRotation().
//
// The RA8876 controls scan direction via bits in DPCR (reg 0x12):
//   bit 7 = PCLK edge (keep 1 = falling, as set during init)
//   bit 6 = Display on (keep 1)
//   bit 4 = HSCAN: 0 = L→R,  1 = R→L
//   bit 3 = VSCAN: 0 = T→B,  1 = B→T
//
// TFT_eSPI_RA8876 rotation modes:
//   0 = landscape (1024 x 600)  L→R, T→B
//   1 = portrait  ( 600 x 1024) not natively supported by RA8876 panel
//   2 = landscape 180°          R→L, B→T
//   3 = portrait  180°          not natively supported
//
// Because the RA8876 panel is fixed at 1024 x 600 and the display
// controller does not have a hardware row/column swap (MV) bit like
// MIPI DCS controllers, true portrait orientation is not available.
// Rotations 0 and 2 flip the scan direction; rotations 1 and 3
// fall back to landscape with horizontal mirror only.
//
// After changing DPCR the _width / _height members are updated so
// that the rest of the TFT_eSPI_RA8876 drawing code uses the correct bounds.

#if defined (RA8876_DRIVER)

  switch (rotation) {

    case 0:   // Landscape, normal (1024 x 600)
      // DPCR: display on + PCLK falling + L→R + T→B = 0xC0
      RA8876_REG(0x12, 0xC0);
      _width  = TFT_WIDTH;   // 1024
      _height = TFT_HEIGHT;  // 600
      break;

    case 1:   // "Portrait" — physically the same panel; mirror horizontally
      // DPCR: PCLK falling + display on + HSCAN R→L = 0xD0
      RA8876_REG(0x12, 0xD0);
      _width  = TFT_WIDTH;   // 1024 (panel cannot rotate 90°)
      _height = TFT_HEIGHT;  // 600
      break;

    case 2:   // Landscape, 180° (R→L, B→T)
      // DPCR: PCLK falling + display on + HSCAN R→L + VSCAN B→T = 0xD8
      RA8876_REG(0x12, 0xD8);
      _width  = TFT_WIDTH;   // 1024
      _height = TFT_HEIGHT;  // 600
      break;

    case 3:   // "Portrait 180°" — mirror vertically only
      // DPCR: PCLK falling + display on + VSCAN B→T = 0xC8
      RA8876_REG(0x12, 0xC8);
      _width  = TFT_WIDTH;   // 1024
      _height = TFT_HEIGHT;  // 600
      break;
  }

#endif  // RA8876_DRIVER
