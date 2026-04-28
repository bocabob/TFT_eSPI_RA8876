/*
 * User_Setup_LCC_Active.h  —  LCC project board selector for TFT_eSPI_RA8876
 *
 * This is the ONE library file to edit when switching hardware targets.
 * Arduino IDE compiles library sources separately from the sketch, so the
 * sketch-side ProjectConfig.h cannot propagate to the library binary.
 * This file is the library-side counterpart to ProjectConfig.h.
 *
 * HOW TO SWITCH BOARDS
 * --------------------
 * Uncomment the line matching your active board and comment the other out.
 * Must stay in sync with ProjectConfig.h in the sketch directory.
 *
 *   v2.4 board  →  SSD1963 8-bit parallel 800×480
 *   v2.7 board  →  RA8876 SPI 1024×600
 */

//#include "Setup104a_RP2040_SSD1963_parallel.h"   // v2.4 board (SSD1963 parallel)
#include "User_Setup_RP2040_RA8876_SPI.h"         // v2.7 board (RA8876 SPI)
