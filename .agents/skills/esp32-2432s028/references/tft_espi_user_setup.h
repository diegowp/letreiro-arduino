// =========================================================================
// Setup para ESP32-2432S028 (CYD - Cheap Yellow Display 2.8" TFT ILI9341)
// Copie este conteúdo para: Arduino/libraries/TFT_eSPI/User_Setup.h
// ou inclua via User_Setup_Select.h
// =========================================================================

#define USER_SETUP_INFO "ESP32-2432S028_CYD_2.8INCH"

// -------------------------------------------------------------------------
// 1. SELEÇÃO DO DRIVER DA TELA
// -------------------------------------------------------------------------
// A maioria das placas ESP32-2432S028 usa ILI9341 ou ILI9341_2.
// ILI9341_2 previne problemas de inversão de cores (BGR vs RGB).
#define ILI9341_2_DRIVER

// Caso a sua placa seja a revisão mais nova com driver ST7789,
// comente a linha acima e descomente a linha abaixo:
// #define ST7789_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// -------------------------------------------------------------------------
// 2. PINAGEM DE HARDWARE DO DISPLAY (ESP32)
// -------------------------------------------------------------------------
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15  // Chip select
#define TFT_DC    2  // Data/Command (RS)
#define TFT_RST  -1  // Conectado ao pino de reset/EN do ESP32

// Pino do Backlight (LED da tela)
#define TFT_BL   21  // Backlight LED
#define TFT_BACKLIGHT_ON HIGH // Nível para ligar backlight

// -------------------------------------------------------------------------
// 3. FONTES CARREGADAS NA MEMÓRIA FLASH
// -------------------------------------------------------------------------
#define LOAD_GLCD   // Fonte 1. Padrão Adafruit 8px
#define LOAD_FONT2  // Fonte 2. Pequena 16px
#define LOAD_FONT4  // Fonte 4. Média 26px
#define LOAD_FONT6  // Fonte 6. Grande 48px numérico
#define LOAD_FONT7  // Fonte 7. 7 segmentos 48px numérico
#define LOAD_FONT8  // Fonte 8. Extra grande 75px numérico
#define LOAD_GFXFF  // FreeFonts (Adafruit_GFX)
#define SMOOTH_FONT // Suporte a fontes antialiasing (.vlw geradas)

// -------------------------------------------------------------------------
// 4. FREQUÊNCIA DO BARRAMENTO SPI
// -------------------------------------------------------------------------
#define SPI_FREQUENCY       40000000 // 40 MHz (máximo estável para o ILI9341)
#define SPI_READ_FREQUENCY  20000000 // 20 MHz para leituras do display

// -------------------------------------------------------------------------
// NOTA SOBRE O TOUCH (XPT2046):
// No hardware do ESP32-2432S028, o XPT2046 está conectado a pinos SPI
// totalmente separados do LCD (CLK: 25, MISO: 39, MOSI: 32, CS: 33).
// Como a TFT_eSPI espera barramento SPI compartilhado, NÃO defina TOUCH_CS
// aqui se for utilizar a biblioteca XPT2046_Touchscreen separadamente.
// -------------------------------------------------------------------------
