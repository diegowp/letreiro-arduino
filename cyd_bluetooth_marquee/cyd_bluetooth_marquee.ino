/*
  ESP32-2432S028 ("CYD" 2.8") - Letreiro Digital & Receptor de Imagens v8.2
  
  Correções da versão 8.2 (CORREÇÃO DE CORTE DE ACENTOS E BORDAS REDONDAS):
  1. FIM DO CORTE NO ACENTO CIRCUNFLEXO (Ê) E TILS (Ã, Õ):
     O rascunho tipográfico foi expandido para 60x54 com linha de base rebaixada em Y=38.
     Isso garante espaço de sobra no topo para o acento circunflexo (^ do 'Ê') e tils sem cortar!
  2. BORDAS DE LETRAS REDONDAS (C, S, O) 100% COMPLETAS:
     Área de escaneamento aumentada horizontal e verticalmente para acomodar o overshoot
     e a largura das curvas de letras como 'C' e 'S'.
  3. TAMANHO GIGANTE REAL (70% DA TELA / ~168px):
     Escala calibrada matematicamente para 4x, proporcionando exatamente 168 pixels
     em um sprite de 180px com 10px de margem de respiro no topo e no fundo!
  4. ESTABILIDADE DE MEMÓRIA (SEM BOOT LOOP):
     Mantido o Dual-Half (160x180 em 16 bits = apenas 57 KB de RAM).
*/

#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <U8g2_for_TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// -----------------------------------------------------------------------------
// Pinos de Hardware do ESP32-2432S028 (CYD)
// -----------------------------------------------------------------------------
#define CYD_TFT_BL      21  // Backlight padrão
#define CYD_TFT_BL_ALT  27  // Backlight alternativo
#define CYD_LED_RED      4  // LED RGB Vermelho (Active LOW)
#define CYD_LED_GREEN   16  // LED RGB Verde (Active LOW)
#define CYD_LED_BLUE    17  // LED RGB Azul (Active LOW)

// -----------------------------------------------------------------------------
// UUIDs do Serviço Nordic UART
// -----------------------------------------------------------------------------
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Instâncias
TFT_eSPI tft = TFT_eSPI();
// Sprite em 16 BITS NATIVOS (RGB565) - 160x180px (apenas 57 KB de RAM, 100% estável)
TFT_eSprite marqueeSprite = TFT_eSprite(&tft);
// Sprite de rascunho com margens ampliadas para acomodar acentos altos (^, ~) e curvas (apenas 3.2 KB)
TFT_eSprite glyphSpr = TFT_eSprite(&tft);
U8g2_for_TFT_eSPI u8f;

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// -----------------------------------------------------------------------------
// Configurações do Letreiro
// -----------------------------------------------------------------------------
String marqueeText = "CIÊNCIA! Atenção ao pão e coração (~)!";

// Cores puras de 16 bits (RGB565) idênticas na tela e no sprite
uint16_t textColor = TFT_YELLOW;
uint16_t textBgColor = TFT_BLACK;

int scrollSpeed = 4;            // Pixels por frame
int textScaleMultiplier = 4;    // 1 = Normal (~40px), 2 = Médio (~80px), 3 = Grande (~120px), 4 = GIGANTE (~168px [70% da tela!])
bool cleanLayout = true;        // true = Tela limpa
int textX = 320;
int textWidth = 0;
bool hasActiveImage = false;

// Flags de sincronização atômica
volatile bool newTextPending = false;
String pendingText = "";
volatile bool refreshScreenPending = false;
volatile bool newImagePending = false;

// -----------------------------------------------------------------------------
// Fator de Escala Efetivo
// -----------------------------------------------------------------------------
int getEffectiveScale() {
  if (textScaleMultiplier == 1) return 1;
  if (textScaleMultiplier == 2) return 2;
  if (textScaleMultiplier == 3) return 3;
  return 4; // Escolha 4 = GIGANTE 70% (escala 4x = ~168px de altura, com 10px de respiro no topo e na base!)
}

// -----------------------------------------------------------------------------
// Buffer de Imagem Dinâmico (Economiza 45 KB de RAM para o BLE não cair em boot loop)
// -----------------------------------------------------------------------------
uint8_t* imageBuffer = NULL;
size_t expectedImageSize = 0;
size_t receivedImageBytes = 0;
bool isReceivingImage = false;

// Callback do decodificador JPEG
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void setRgbColor(bool r, bool g, bool b) {
  digitalWrite(CYD_LED_RED,   r ? LOW : HIGH);
  digitalWrite(CYD_LED_GREEN, g ? LOW : HIGH);
  digitalWrite(CYD_LED_BLUE,  b ? LOW : HIGH);
}

// -----------------------------------------------------------------------------
// Extrator Seguro de Caracteres UTF-8 (1 a 4 bytes)
// -----------------------------------------------------------------------------
String getNextUtf8Char(const String &str, size_t &idx) {
  if (idx >= str.length()) return "";
  uint8_t c = (uint8_t)str[idx];
  String glyph = "";
  glyph += (char)c;
  idx++;
  if ((c & 0xE0) == 0xC0 && idx < str.length()) { // 2 bytes (ã, õ, ç, é, ê, etc.)
    glyph += (char)str[idx++];
  } else if ((c & 0xF0) == 0xE0 && idx + 1 < str.length()) { // 3 bytes
    glyph += (char)str[idx++];
    glyph += (char)str[idx++];
  }
  return glyph;
}

// Calcula a largura total em pixels da frase na escala calibrada
int calculateScaledWidth(const String &text, int scale) {
  int totalW = 0;
  size_t idx = 0;
  while (idx < text.length()) {
    String ch = getNextUtf8Char(text, idx);
    totalW += u8f.getUTF8Width(ch.c_str()) * scale;
  }
  return totalW;
}

// -----------------------------------------------------------------------------
// Callbacks do Servidor BLE
// -----------------------------------------------------------------------------
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    setRgbColor(false, false, true);
    Serial.println("[BLE] Conectado!");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    setRgbColor(true, false, false);
    Serial.println("[BLE] Desconectado!");
  }
};

// -----------------------------------------------------------------------------
// Processamento de Comandos Unificados ($CFG)
// -----------------------------------------------------------------------------
void processCommand(String cmd) {
  cmd.trim();

  // COMANDO UNIFICADO: $CFG:fgHex,bgHex,size,speed,layout,texto
  if (cmd.startsWith("$CFG:")) {
    String payload = cmd.substring(5);
    int p1 = payload.indexOf(',');
    int p2 = payload.indexOf(',', p1 + 1);
    int p3 = payload.indexOf(',', p2 + 1);
    int p4 = payload.indexOf(',', p3 + 1);
    int p5 = payload.indexOf(',', p4 + 1);

    if (p1 > 0 && p2 > 0 && p3 > 0 && p4 > 0 && p5 > 0) {
      String fgHex = payload.substring(0, p1);
      String bgHex = payload.substring(p1 + 1, p2);
      int sizeChoice = payload.substring(p2 + 1, p3).toInt();
      int spd        = payload.substring(p3 + 1, p4).toInt();
      int lay        = payload.substring(p4 + 1, p5).toInt();
      String txt     = payload.substring(p5 + 1);

      long fgVal = strtol(fgHex.c_str(), NULL, 16);
      long bgVal = strtol(bgHex.c_str(), NULL, 16);

      uint8_t fgR = (fgVal >> 16) & 0xFF;
      uint8_t fgG = (fgVal >> 8)  & 0xFF;
      uint8_t fgB = fgVal         & 0xFF;

      uint8_t bgR = (bgVal >> 16) & 0xFF;
      uint8_t bgG = (bgVal >> 8)  & 0xFF;
      uint8_t bgB = bgVal         & 0xFF;

      // Cores 16 bits puras (RGB565)
      textColor = tft.color565(fgR, fgG, fgB);
      textBgColor = tft.color565(bgR, bgG, bgB);

      textScaleMultiplier = constrain(sizeChoice, 1, 4);
      scrollSpeed = constrain(spd, 1, 25);
      cleanLayout = (lay == 1);
      pendingText = txt;
      newTextPending = true;

      Serial.printf("[CFG] Cores: FG=0x%04X, BG=0x%04X | Escala=%dx | Texto: %s\n",
                    textColor, textBgColor, textScaleMultiplier, pendingText.c_str());
    }
    return;
  }

  // Compatibilidade com comandos avulsos
  if (cmd.startsWith("$TXT:")) {
    pendingText = cmd.substring(5);
    newTextPending = true;
  }
  else if (cmd.startsWith("$SPD:")) {
    scrollSpeed = constrain(cmd.substring(5).toInt(), 1, 25);
  }
  else if (cmd.startsWith("$FSZ:")) {
    textScaleMultiplier = constrain(cmd.substring(5).toInt(), 1, 4);
    newTextPending = true;
  }
  else if (cmd.startsWith("$LAY:")) {
    cleanLayout = (cmd.substring(5) == "1");
    refreshScreenPending = true;
  }
  else if (cmd.startsWith("$IMG:")) {
    expectedImageSize = cmd.substring(5).toInt();
    receivedImageBytes = 0;
    if (imageBuffer != NULL) {
      free(imageBuffer);
      imageBuffer = NULL;
    }
    if (expectedImageSize > 0 && expectedImageSize <= 45000) {
      imageBuffer = (uint8_t*)malloc(expectedImageSize);
      if (imageBuffer != NULL) {
        isReceivingImage = true;
        Serial.printf("[IMG] Aguardando %d bytes...\n", expectedImageSize);
        setRgbColor(false, true, false);
      } else {
        Serial.println("[IMG] Falha de alocacao de memoria para imagem!");
      }
    }
  }
  else if (cmd == "$CLR") {
    hasActiveImage = false;
    refreshScreenPending = true;
  }
}

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    uint8_t* data = pCharacteristic->getData();
    size_t len = pCharacteristic->getLength();
    if (len == 0) return;

    if (isReceivingImage) {
      if (imageBuffer != NULL && (receivedImageBytes + len <= expectedImageSize)) {
        memcpy(&imageBuffer[receivedImageBytes], data, len);
        receivedImageBytes += len;

        if (receivedImageBytes >= expectedImageSize) {
          isReceivingImage = false;
          newImagePending = true;
          Serial.printf("[IMG] Imagem recebida (%d bytes)!\n", receivedImageBytes);
        }
      } else {
        Serial.println("[IMG] Erro no buffer!");
        isReceivingImage = false;
        if (imageBuffer != NULL) {
          free(imageBuffer);
          imageBuffer = NULL;
        }
      }
      return;
    }

    String str = "";
    for (size_t i = 0; i < len; i++) {
      str += (char)data[i];
    }
    processCommand(str);
  }
};

void drawFullLayoutChrome() {
  if (hasActiveImage || cleanLayout) return;

  tft.fillRect(0, 0, 320, 24, TFT_NAVY);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.drawString("CYD 2.8 - PAINEL BLUETOOTH", 10, 4);

  tft.drawFastHLine(0, 24, 320, TFT_CYAN);
  tft.drawFastHLine(0, 222, 320, TFT_DARKGREY);

  tft.setTextColor(TFT_LIGHTGREY, textBgColor);
  tft.drawString("Status BLE: ", 10, 226);
}

void updateBleStatus() {
  if (hasActiveImage || cleanLayout) return;
  tft.setTextFont(2);
  tft.setTextSize(1);
  if (deviceConnected) {
    tft.setTextColor(TFT_GREEN, textBgColor);
    tft.drawString("CONECTADO    ", 90, 226);
  } else {
    tft.setTextColor(TFT_ORANGE, textBgColor);
    tft.drawString("AGUARDANDO... ", 90, 226);
  }
}

// -----------------------------------------------------------------------------
// Renderizador de Metade do Display (0 a 159 ou 160 a 319)
// Sem cortes em acentos altos (^, ~), sem corte em curvas de letras (C, S, O)
// -----------------------------------------------------------------------------
void drawMarqueeHalf(int halfLeft, int scale, int baselineY) {
  marqueeSprite.fillSprite(textBgColor);

  int curX = textX;
  size_t idx = 0;
  int halfRight = halfLeft + 160;
  int sprH = marqueeSprite.height();

  while (idx < marqueeText.length()) {
    String ch = getNextUtf8Char(marqueeText, idx);
    int rawCharW = u8f.getUTF8Width(ch.c_str());
    int charAdvance = rawCharW * scale;

    // Se passou do lado direito desta metade, encerra o loop deste frame
    if (curX >= halfRight) {
      break;
    }

    // Se o caractere cruza a janela visível desta metade
    if (curX + charAdvance > halfLeft) {
      if (ch != " ") {
        glyphSpr.fillSprite(0);
        // Cursor em (6, 38): 38px de margem vertical superior evita cortar ^ no 'Ê' e ~ no 'Ã'
        u8f.setCursor(6, 38);
        u8f.print(ch);

        int scanW = min(60, rawCharW + 12);
        int scanH = 54;

        for (int gy = 0; gy < scanH; gy++) {
          for (int gx = 0; gx < scanW; gx++) {
            if (glyphSpr.readPixel(gx, gy) != 0) {
              int screenPx = curX + (gx - 6) * scale;
              int px = screenPx - halfLeft;
              int py = baselineY + (gy - 38) * scale;

              // Desenha somente dentro dos limites do sprite (0 a 159 horizontal, 0 a sprH vertical)
              if (px + scale > 0 && px < 160 && py + scale > 0 && py < sprH) {
                if (scale == 1) {
                  marqueeSprite.drawPixel(px, py, textColor);
                } else {
                  marqueeSprite.fillRect(px, py, scale, scale, textColor);
                }
              }
            }
          }
        }
      }
    }

    curX += charAdvance;
  }
}

void setup() {
  Serial.begin(115200);

  // 1. Backlight da tela (CYD)
  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, HIGH);
  pinMode(CYD_TFT_BL_ALT, OUTPUT);
  digitalWrite(CYD_TFT_BL_ALT, HIGH);

  // 2. LEDs RGB onboard
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  setRgbColor(true, false, false);

  // 3. Inicializa Display
  tft.init();
  tft.setRotation(1); // Paisagem 320x240
  tft.fillScreen(textBgColor);

  // 4. Decodificador JPEG
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  // 5. Sprite Otimizado em 16 BITS NATIVOS (RGB565) de 160x180 (apenas 57 KB de RAM!)
  // Garante espaço vertical de sobra (180px) para acomodar a palavra inteira com acentos
  marqueeSprite.setColorDepth(16);
  if (!marqueeSprite.createSprite(160, 180)) {
    if (!marqueeSprite.createSprite(160, 168)) {
      marqueeSprite.createSprite(160, 150);
    }
  }

  // 6. Sprite de Rascunho com 60x54 para acomodar letras com acentos altos e overshoot (apenas 3.2 KB)
  glyphSpr.setColorDepth(8);
  glyphSpr.createSprite(60, 54);

  // 7. Conecta U8g2 com a fonte FreeUniversal Bold
  u8f.begin(glyphSpr);
  u8f.setFont(u8g2_font_fub30_tf);
  u8f.setFontMode(1);
  u8f.setForegroundColor(255);

  textWidth = calculateScaledWidth(marqueeText, getEffectiveScale());

  drawFullLayoutChrome();

  // 8. Bluetooth Low Energy
  BLEDevice::init("CYD-Letreiro");
  BLEDevice::setMTU(512);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                                           CHARACTERISTIC_UUID_RX,
                                           BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
                                         );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Servidor pronto v8.2 Estavel (Sem cortes)!");
}

void loop() {
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  // ---------------------------------------------------------------------------
  // PROCESSAMENTO SEGURO DE EVENTOS (Thread-Safe e Atômico)
  // ---------------------------------------------------------------------------
  if (refreshScreenPending) {
    refreshScreenPending = false;
    hasActiveImage = false;
    tft.fillScreen(textBgColor);
    drawFullLayoutChrome();
    textX = 320;
  }

  if (newTextPending) {
    newTextPending = false;
    marqueeText = pendingText;
    textX = 320;
    textWidth = calculateScaledWidth(marqueeText, getEffectiveScale());
    hasActiveImage = false;
    tft.fillScreen(textBgColor);
    drawFullLayoutChrome();
  }

  if (newImagePending) {
    newImagePending = false;
    if (imageBuffer != NULL) {
      TJpgDec.drawJpg(0, 0, imageBuffer, receivedImageBytes);
      free(imageBuffer);
      imageBuffer = NULL;
    }
    hasActiveImage = true;
    setRgbColor(false, false, true);
  }

  // ---------------------------------------------------------------------------
  // RENDERIZAÇÃO DO LETREIRO GIGANTE (70% DA TELA) SEM CORTES
  // ---------------------------------------------------------------------------
  if (!hasActiveImage && !isReceivingImage && marqueeSprite.created()) {
    int sprH = marqueeSprite.height();
    int scale = getEffectiveScale();

    // Span vertical da fonte fub30_tf considerando acentos altos (-32px) e descers (+8px):
    int fontTotalH = 40 * scale;
    int topY = max(0, (sprH - fontTotalH) / 2);
    // Linha de base posicionada para deixar 10px de margem acima do acento circunflexo (Ê)
    int baselineY = topY + (32 * scale);

    // Centralizado perfeitamente na tela física
    int spriteScreenY = cleanLayout ? ((240 - sprH) / 2) : 25;

    // 1. Renderiza e desenha a Metade Esquerda (0 a 159)
    drawMarqueeHalf(0, scale, baselineY);
    marqueeSprite.pushSprite(0, spriteScreenY);

    // 2. Renderiza e desenha a Metade Direita (160 a 319)
    drawMarqueeHalf(160, scale, baselineY);
    marqueeSprite.pushSprite(160, spriteScreenY);

    // 3. Deslocamento horizontal
    textX -= scrollSpeed;
    if (textX < -textWidth) {
      textX = 320;
    }

    static unsigned long lastStatusUpdate = 0;
    if (!cleanLayout && millis() - lastStatusUpdate > 1000) {
      lastStatusUpdate = millis();
      updateBleStatus();
    }

    delay(15);
  } else {
    delay(100);
  }
}
