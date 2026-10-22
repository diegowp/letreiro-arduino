/*
  ESP32-2432S028 ("CYD" 2.8") - Dashboard Tátil Interativo
  Exemplo de interface com botões de toque, controle de estados do LED RGB
  e debounce simples para telas sensíveis ao toque resistivas.
  
  Bibliotecas:
  - TFT_eSPI (Bodmer)
  - XPT2046_Touchscreen (Paul Stoffregen)
*/

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Pinos CYD
#define CYD_TFT_BL    21
#define CYD_TFT_BL_ALT 27
#define CYD_LED_RED    4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE  17

// Pinos SPI Touch XPT2046
#define XPT2046_IRQ   36
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CLK   25
#define XPT2046_CS    33

TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// Estrutura de Botão
struct Button {
  int x, y, w, h;
  const char* label;
  uint16_t color;
  bool state;
};

Button btnRed   = { 20,  70, 80, 50, "VERMELHO", TFT_RED,   false };
Button btnGreen = { 120, 70, 80, 50, "VERDE",    TFT_GREEN, false };
Button btnBlue  = { 220, 70, 80, 50, "AZUL",     TFT_BLUE,  false };

unsigned long lastTouchTime = 0;

void drawButton(Button &btn) {
  uint16_t bgColor = btn.state ? btn.color : TFT_DARKGREY;
  uint16_t border = btn.state ? TFT_WHITE : TFT_LIGHTGREY;

  tft.fillRoundRect(btn.x, btn.y, btn.w, btn.h, 8, bgColor);
  tft.drawRoundRect(btn.x, btn.y, btn.w, btn.h, 8, border);

  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextDatum(MC_DATUM); // Centro do texto
  tft.setTextSize(1);
  tft.drawString(btn.label, btn.x + (btn.w / 2), btn.y + (btn.h / 2) - 6);
  tft.drawString(btn.state ? "[LIGADO]" : "[DESLIGADO]", btn.x + (btn.w / 2), btn.y + (btn.h / 2) + 8);
}

void applyLeds() {
  // Lógica Active-LOW
  digitalWrite(CYD_LED_RED,   btnRed.state   ? LOW : HIGH);
  digitalWrite(CYD_LED_GREEN, btnGreen.state ? LOW : HIGH);
  digitalWrite(CYD_LED_BLUE,  btnBlue.state  ? LOW : HIGH);
}

bool checkButtonPress(Button &btn, int touchX, int touchY) {
  return (touchX >= btn.x && touchX <= (btn.x + btn.w) &&
          touchY >= btn.y && touchY <= (btn.y + btn.h));
}

void setup() {
  Serial.begin(115200);

  // Backlight e LEDs
  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, HIGH);
  pinMode(CYD_TFT_BL_ALT, OUTPUT);
  digitalWrite(CYD_TFT_BL_ALT, HIGH);

  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  applyLeds();

  // Display
  tft.init();
  tft.setRotation(1); // Paisagem 320x240
  tft.fillScreen(TFT_BLACK);

  // Touch
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);

  // Layout UI
  tft.fillRect(0, 0, 320, 40, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);
  tft.drawString("Painel de Controle CYD", 160, 20);

  drawButton(btnRed);
  drawButton(btnGreen);
  drawButton(btnBlue);

  tft.drawFastHLine(10, 150, 300, TFT_DARKGREY);
  tft.setTextDatum(TL_DATUM);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("Status do Toque:", 20, 165);
}

void loop() {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    int tx = map(p.x, 200, 3700, 0, 320);
    int ty = map(p.y, 240, 3800, 0, 240);
    tx = constrain(tx, 0, 319);
    ty = constrain(ty, 0, 239);

    // Debounce simples de 250ms
    if (millis() - lastTouchTime > 250) {
      bool changed = false;

      if (checkButtonPress(btnRed, tx, ty)) {
        btnRed.state = !btnRed.state;
        drawButton(btnRed);
        changed = true;
      } else if (checkButtonPress(btnGreen, tx, ty)) {
        btnGreen.state = !btnGreen.state;
        drawButton(btnGreen);
        changed = true;
      } else if (checkButtonPress(btnBlue, tx, ty)) {
        btnBlue.state = !btnBlue.state;
        drawButton(btnBlue);
        changed = true;
      }

      if (changed) {
        applyLeds();
        lastTouchTime = millis();
      }

      // Atualiza coordenadas na tela
      char coords[32];
      snprintf(coords, sizeof(coords), "X: %3d | Y: %3d | Z: %4d   ", tx, ty, p.z);
      tft.setTextDatum(TL_DATUM);
      tft.setTextSize(2);
      tft.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
      tft.drawString(coords, 20, 190);
    }
  }
}
