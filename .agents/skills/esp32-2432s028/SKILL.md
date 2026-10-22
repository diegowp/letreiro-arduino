---
name: esp32-2432s028
description: >-
  Especialista em desenvolvimento de firmware para a placa ESP32-2432S028 (CYD - Cheap Yellow Display)
  com tela TFT 2.8" (ILI9341/ST7789) e touch XPT2046. Use esta skill ao desenvolver projetos, sketches Arduino,
  configurar a biblioteca TFT_eSPI, integrar o touch XPT2046, controlar o LED RGB onboard, sensor LDR,
  áudio ou cartão SD com o core esp32 da Espressif Systems.
---

# Skill: Desenvolvimento ESP32-2432S028 ("CYD" 2.8")

Esta skill fornece diretrizes técnicas, arquitetura de pinos, configurações de biblioteca e templates de código para o módulo **ESP32-2432S028** (popularmente conhecido como **CYD - Cheap Yellow Display**), equipado com tela TFT colorida de 2.8 polegadas (320x240), touch resistivo XPT2046, LED RGB, sensor LDR e slot MicroSD.

---

## 1. Configuração do Ambiente (Arduino IDE / CLI)

### Placa e Core Espressif
- **Gerenciador de Placas URL**: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- **Pacote**: `esp32 by Espressif Systems` (versão 2.x ou 3.x)
- **Seleção de Placa**: `ESP32 Dev Module`
- **Configurações recomendadas no menu Tools/Ferramentas**:
  - **CPU Frequency**: `240MHz (WiFi/BT)`
  - **Flash Frequency**: `80MHz`
  - **Flash Mode**: `QIO`
  - **Flash Size**: `4MB (32Mb)`
  - **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)` (recomendado para projetos com muitas fontes ou bibliotecas gráficas)
  - **Core Debug Level**: `None` (ou `Verbose` para depuração)
  - **Upload Speed**: `921600` (se falhar, use `115200`)

---

## 2. Pinout Completo da Placa ESP32-2432S028

### 2.1 Display TFT (Barramento SPI 1)
| Sinal | GPIO ESP32 | Função / Detalhes |
| :--- | :--- | :--- |
| **TFT_MISO** | `GPIO 12` | MISO (muitas vezes não conectado fisicamente no LCD) |
| **TFT_MOSI** | `GPIO 13` | Linha de dados SPI (SDA / SDI) |
| **TFT_SCLK** | `GPIO 14` | Clock SPI (SCK / SCL) |
| **TFT_CS** | `GPIO 15` | Chip Select do Display |
| **TFT_DC** | `GPIO 2` | Data / Command (RS) |
| **TFT_RST** | `-1` (EN) | Ligado ao Reset geral da placa |
| **TFT_BL** | `GPIO 21` | Backlight LED (HIGH = ligado; suporta PWM LEDC) |

> [!IMPORTANT]
> **Atenção ao Backlight (GPIO 21)**: O pino 21 aciona o backlight da tela. Se este pino não for colocado em nível lógico ALTO (`digitalWrite(21, HIGH)` ou configurado via TFT_eSPI / LEDC PWM), **a tela permanecerá completamente preta** mesmo que o código esteja rodando!

### 2.2 Touch Screen Resistivo (XPT2046) - Barramento SPI 2
> [!WARNING]
> **Peculiaridade de Hardware Crítica do CYD**:
> O controlador de touch XPT2046 **NÃO** compartilha os pinos SPI do display na PCB!
> Ele está conectado a pinos SPI independentes:
> - **T_CLK**: `GPIO 25`
> - **T_MISO (DOUT)**: `GPIO 39` (Sensor VN - Somente Entrada)
> - **T_MOSI (DIN)**: `GPIO 32`
> - **T_CS**: `GPIO 33`
> - **T_IRQ**: `GPIO 36` (Sensor VP - Somente Entrada, opcional)
>
> Por essa razão, a função nativa `tft.getTouch()` do TFT_eSPI (que presume mesmo barramento SPI) não funciona diretamente no CYD original sem pontes de solda. **A melhor prática é utilizar a biblioteca `XPT2046_Touchscreen` em uma instância secundária do SPI (VSPI/HSPI) ou bitbang.**

### 2.3 Periféricos Onboard Adicionais
| Periférico | GPIO ESP32 | Lógica / Comportamento |
| :--- | :--- | :--- |
| **LED RGB - Vermelho (R)** | `GPIO 4` | **Ativo em LOW** (0 = Aceso, 1 = Apagado) |
| **LED RGB - Verde (G)** | `GPIO 16` | **Ativo em LOW** (0 = Aceso, 1 = Apagado) |
| **LED RGB - Azul (B)** | `GPIO 17` | **Ativo em LOW** (0 = Aceso, 1 = Apagado) |
| **Sensor LDR (Luz)** | `GPIO 34` | Entrada analógica ADC1 (`analogRead(34)`) |
| **Áudio / Speaker DAC** | `GPIO 26` | Saída analógica DAC / amplificador PAM8302 |
| **MicroSD - CS** | `GPIO 5` | Barramento VSPI dedicado ao cartão SD |
| **MicroSD - SCK** | `GPIO 18` | SCK do SD |
| **MicroSD - MISO** | `GPIO 19` | MISO do SD |
| **MicroSD - MOSI** | `GPIO 23` | MOSI do SD |
| **Conector P3 (I2C / IO)** | `GPIO 22` (SDA), `GPIO 27` (SCL) | Porta I2C externa ou GPIOs livres |
| **Conector CN1 (Serial)** | `GPIO 1` (TXD0), `GPIO 3` (RXD0) | Conexão serial primária e gravação USB |

---

## 3. Configuração da Biblioteca `TFT_eSPI`

Para compilar projetos com `TFT_eSPI` (por Bodmer), deve-se configurar o arquivo `User_Setup.h` ou usar uma definição personalizada via `User_Setup_Select.h`.

Consulte o template completo pronto para uso em [tft_espi_user_setup.h](./references/tft_espi_user_setup.h).

### Resumo das Diretivas Críticas para `User_Setup.h`:
```c
#define USER_SETUP_INFO "ESP32-2432S028_CYD"

// Driver do Display (Geralmente ILI9341 ou ILI9341_2)
#define ILI9341_2_DRIVER // Use ILI9341_2_DRIVER para evitar inversão de cores no CYD

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

#define SPI_FREQUENCY       40000000 // 40MHz
#define SPI_READ_FREQUENCY  20000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
```

> [!TIP]
> **Cores Invertidas?**
> Se o fundo branco parecer preto ou as cores ficarem estranhas:
> 1. Alterne entre `#define ILI9341_DRIVER` e `#define ILI9341_2_DRIVER`.
> 2. Ou no código `setup()`, chame `tft.invertDisplay(true);` ou `tft.invertDisplay(false);`.
> 3. Algumas revisões novas vêm com o driver **ST7789**. Se a tela não responder, comente o ILI9341 e descomente `#define ST7789_DRIVER`.

---

## 4. Arquitetura de Código Recomendada

### 4.1 Bibliotecas Necessárias
Instale pelo Gerenciador de Bibliotecas da Arduino IDE:
1. `TFT_eSPI` (por Bodmer)
2. `XPT2046_Touchscreen` (por Paul Stoffregen)

### 4.2 Padrão de Inicialização de Display + Touch Dual SPI
```cpp
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Pinos do Touch XPT2046 (Barramento SPI 2)
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Instâncias
TFT_eSPI tft = TFT_eSPI();
SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

// Pinos do LED RGB e Backlight
#define CYD_LED_RED   4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE  17
#define CYD_TFT_BL    21
#define CYD_LDR       34

void setup() {
  Serial.begin(115200);

  // 1. Configurar Backlight
  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, HIGH);

  // 2. Configurar LEDs onboard (Active LOW: HIGH = desligado)
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  digitalWrite(CYD_LED_RED, HIGH);
  digitalWrite(CYD_LED_GREEN, HIGH);
  digitalWrite(CYD_LED_BLUE, HIGH);

  // 3. Inicializar TFT
  tft.init();
  tft.setRotation(1); // 1 = Landscape (320x240)
  tft.fillScreen(TFT_BLACK);

  // 4. Inicializar barramento SPI separado para o Touch
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSPI);
  ts.setRotation(1); // Deve corresponder à rotação do TFT

  Serial.println("ESP32-2432S028 inicializado com sucesso!");
}
```

### 4.3 Mapeamento e Calibração de Coordenadas do Touch
Os valores brutos retornados pelo XPT2046 variam entre ~200 e ~3800. Para converter em coordenadas de tela de 0 a 320 (X) e 0 a 240 (Y):
```cpp
void readTouchCoordinates(int &screenX, int &screenY) {
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    
    // Calibração típica para Landscape (setRotation(1)):
    // Ajuste estes limites de acordo com sua unidade específica
    screenX = map(p.x, 200, 3700, 0, 320);
    screenY = map(p.y, 240, 3800, 0, 240);
    
    // Limita os valores aos limites da tela
    screenX = constrain(screenX, 0, 319);
    screenY = constrain(screenY, 0, 239);
  } else {
    screenX = -1;
    screenY = -1;
  }
}
```

---

## 5. Controle de Brilho Dinâmico via PWM (LDR + Backlight)

O pino `CYD_TFT_BL` (GPIO 21) pode ser controlado via PWM do ESP32 (`ledc`) para ajustar o brilho automaticamente com base no sensor de luz LDR (GPIO 34):

```cpp
void setupBacklightPWM() {
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(CYD_TFT_BL, 5000, 8); // Frequência: 5kHz, Resolução: 8 bits (0-255)
    ledcWrite(CYD_TFT_BL, 255);       // Brilho máximo inicial
  #else
    ledcSetup(0, 5000, 8);           // Canal 0, 5kHz, 8 bits
    ledcAttachPin(CYD_TFT_BL, 0);
    ledcWrite(0, 255);
  #endif
}

void adjustBrightnessWithLDR() {
  int ldrValue = analogRead(CYD_LDR); // 0 (escuro) a 4095 (muito claro)
  // Mapeia para nível de brilho PWM (mínimo 30 para a tela não apagar completamente)
  int brightness = map(ldrValue, 0, 4095, 30, 255);
  
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(CYD_TFT_BL, brightness);
  #else
    ledcWrite(0, brightness);
  #endif
}
```

---

## 6. Guia de Resolução de Problemas (Troubleshooting)

| Sintoma | Causa Mais Provável | Solução |
| :--- | :--- | :--- |
| **Tela totalmente preta / sem luz** | Backlight desligado | Garanta `pinMode(21, OUTPUT); digitalWrite(21, HIGH);` no início do `setup()`. |
| **Tela totalmente branca / sem gráficos** | Pinos SPI incorretos ou driver errado | Verifique se `TFT_MOSI=13`, `TFT_SCLK=14`, `TFT_CS=15`, `TFT_DC=2`. Teste `#define ILI9341_2_DRIVER` ou `ST7789_DRIVER`. |
| **Cores invertidas (azul no lugar de vermelho)** | Configuração de inversão do painel | Use `tft.invertDisplay(true);` ou alterne entre `ILI9341_DRIVER` e `ILI9341_2_DRIVER`. |
| **Touch não responde ou trava o sketch** | Conflito no barramento SPI | Não use `tft.getTouch()`. Use `XPT2046_Touchscreen` com instância de `SPIClass touchSPI(VSPI)` nos pinos 25, 39, 32, 33. |
| **Coordenadas do Touch desalinhadas** | Rotação divergente entre TFT e Touch | Chame `tft.setRotation(r);` e `ts.setRotation(r);` com o mesmo valor e calibre `map(p.x, ...)` e `map(p.y, ...)`. |
| **Erro de compilação: falta de espaço de memória** | Esquema de partição padrão de 1.2MB insuficiente | No menu da IDE, altere `Partition Scheme` para `Huge APP (3MB No OTA/1MB SPIFFS)`. |
| **LED RGB onboard sempre aceso ou com cor invertida** | Lógica Active-LOW esquecida | Os pinos 4, 16 e 17 devem receber `HIGH` para desligar e `LOW` para ligar. |

---

## 7. Exemplos Prontos na Skill

- [Hardware Test Completo](./examples/cyd_hardware_test/cyd_hardware_test.ino): Teste gráfico, touch drawing, LED RGB e leitura do LDR.
- [Interface Touch Interativa](./examples/cyd_touch_ui/cyd_touch_ui.ino): Exemplo de dashboard com botões táteis, feedback visual e controle de estado.
- [User_Setup.h de Referência](./references/tft_espi_user_setup.h): Arquivo de configuração de pinos para a pasta da biblioteca TFT_eSPI.
- [Pinout e Conectores Detalhados](./references/pinout_hardware.md): Esquema dos conectores CN1, P2 e P3.
