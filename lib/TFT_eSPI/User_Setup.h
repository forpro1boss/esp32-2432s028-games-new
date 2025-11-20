// User_Setup for 2.8" ILI9341-based displays (TPM408-2.8 assumed)
// Adjust pin mappings below to match your wiring.

#define ILI9341_DRIVER

// SPI pins (ESP32 default SPI pins shown — change as needed)
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5  // Chip select control pin
#define TFT_DC   16 // Data/Command control pin
#define TFT_RST  17 // Reset pin (could connect to Arduino RESET)
#define TFT_BL   21  // Backlight control (TPM408 shows IO21)

// Touch controller chip select (XPT2046 typical)
// According to your board labels: TP CS = IO33
// Change to the GPIO you wired for touch CS if different.
#define TOUCH_CS 33

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6

#define SMOOTH_FONT

// Display resolution
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// If your display uses different driver (e.g., ST7789), replace above

