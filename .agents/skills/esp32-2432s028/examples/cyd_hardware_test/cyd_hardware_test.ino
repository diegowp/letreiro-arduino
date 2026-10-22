/*
  ESP32-2432S028 ("CYD" - Cheap Yellow Display 2.8")
  Hardware Test Completo: TFT_eSPI + XPT2046 Touch + LED RGB + Sensor LDR
  
  Bibliotecas necessárias:
  - TFT_eSPI (por Bodmer) -> configurada com os pinos da placa em User_Setup.h
  - XPT2046_Touchscreen (por Paul Stoffregen)
*/

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// -----------------------------------------------------------------------------
// Pinos de Hardware do ESP32-2432S028
// -----------------------------------------------------------------------------
#define CYD_TFT_BL    21  // Backlight LED (GPIO 21 na maioria, GPIO 27 em algumas revisões)
#define CYD_TFT_BL_ALT 27 // Backlight alternativo
#define CYD_LED_RED    4  // LED RGB Vermelho (Active LOW)
#define CYD_LED_GREEN 16  // LED RGB Verde (Active LOW)
#define CYD_LED_BLUE  17  // LED RGB Azul (Active LOW)
#define CYD_LDR       34  // Sensor de Luminosidade (ADC1)

// Pinos do Touch XPT2046 (Barramento SPI 2)
#define XPT2046_IRQ   36
#define XPT2046_MOSI  32
#define XPT2046_MISO  39
#define XPT2046_CLK   25
#define XPT2046_CS    33

// Instâncias
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// Variáveis de controle
unsigned long lastSensorUpdate = 0;
int currentColorStep = 0;

void setRgbColor(bool r, bool g, bool b) {
  // O LED RGB onboard do CYD é ativo em nível BAIXO (0 = aceso, 1 = apagado)
  digitalWrite(CYD_LED_RED,   r ? LOW : HIGH);
  digitalWrite(CYD_LED_GREEN, g ? LOW : HIGH);
  digitalWrite(CYD_LED_BLUE,  b ? LOW : HIGH);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n[CYD] Iniciando teste de hardware ESP32-2432S028...");

  // 1. Inicializa pinos dos LEDs RGB (desligados no início)
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  setRgbColor(false, false, false);

  // 2. Inicializa o Backlight da tela (ativa GPIO 21 e 27 para cobrir todas as revisões de CYD)
  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, HIGH);
  pinMode(CYD_TFT_BL_ALT, OUTPUT);
  digitalWrite(CYD_TFT_BL_ALT, HIGH);

  // 3. Inicializa o Display TFT
  tft.init();
  tft.setRotation(1); // Modo paisagem (Landscape 320x240)
  tft.fillScreen(TFT_BLACK);

  // 4. Inicializa o barramento SPI separado do Touch XPT2046
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSPI);
  ts.setRotation(1); // Sincroniza orientação com a tela

  // 5. Desenha interface básica
  drawStaticUI();
  Serial.println("[CYD] Hardware pronto para interagir!");
}

void drawStaticUI() {
  // Cabeçalho
  tft.fillRect(0, 0, 320, 32, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(2);
  tft.drawString("ESP32-2432S028 CYD", 10, 8);

  // Área de status do sensor
  tft.drawRoundRect(10, 40, 300, 50, 4, TFT_DARKGREY);
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Sensor de Luz (LDR):", 20, 48);

  // Área de toque para desenho
  tft.drawRoundRect(10, 100, 300, 125, 4, TFT_DARKCYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Area de Toque: Toque para desenhar e testar", 20, 106);
}

void loop() {
  // 1. Leitura e tratamento do Touch
  if (ts.touched()) {
    TS_Point p = ts.getPoint();

    // Mapeamento das coordenadas brutas (aprox. 200..3800) para pixels (0..320, 0..240)
    int x = map(p.x, 200, 3700, 0, 320);
    int y = map(p.y, 240, 3800, 0, 240);
    x = constrain(x, 0, 319);
    y = constrain(y, 0, 239);

    // Se o toque ocorrer dentro da área de desenho:
    if (x >= 12 && x <= 308 && y >= 120 && y <= 220) {
      tft.fillCircle(x, y, 3, TFT_YELLOW);
    }

    // Acende LED Azul enquanto a tela for pressionada
    setRgbColor(false, false, true);
  } else {
    // Quando solto, mantém a cor do ciclo automático
    updateLedCycle();
  }

  // 2. Atualiza sensor LDR a cada 500ms
  if (millis() - lastSensorUpdate >= 500) {
    lastSensorUpdate = millis();
    updateLDRDisplay();
  }
}

void updateLedCycle() {
  static unsigned long lastLedStep = 0;
  if (millis() - lastLedStep > 1000) {
    lastLedStep = millis();
    currentColorStep = (currentColorStep + 1) % 3;
    if (currentColorStep == 0) setRgbColor(true, false, false);      // Vermelho
    else if (currentColorStep == 1) setRgbColor(false, true, false); // Verde
    else setRgbColor(false, false, true);                            // Azul
  }
}

void updateLDRDisplay() {
  int ldrValue = analogRead(CYD_LDR); // 0 a 4095
  int barWidth = map(ldrValue, 0, 4095, 0, 150);
  barWidth = constrain(barWidth, 0, 150);

  // Exibe valor numérico
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  char buf[20];
  snprintf(buf, sizeof(buf), "%4d / 4095", ldrValue);
  tft.drawString(buf, 20, 64);

  // Barra gráfica de intensidade de luz
  tft.fillRect(150, 66, 150, 14, TFT_BLACK);
  tft.fillRect(150, 66, barWidth, 14, TFT_ORANGE);
  tft.drawRect(150, 66, 150, 14, TFT_WHITE);
}
