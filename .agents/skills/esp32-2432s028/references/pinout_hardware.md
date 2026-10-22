# Pinout e Hardware da Placa ESP32-2432S028 ("CYD")

O módulo **ESP32-2432S028** integra em uma única PCB um microcontrolador ESP32-WROOM-32 (ou ESP32-D0WDQ6), tela LCD TFT de 2.8 polegadas, painel de toque resistivo XPT2046, slot MicroSD, LED RGB, sensor de luminosidade LDR, amplificador de áudio com conector para alto-falante e portas de expansão.

---

## 1. Mapeamento Geral de GPIOs

| GPIO | Tipo | Conectado Na Placa | Observações Importantes |
| :---: | :---: | :--- | :--- |
| **0** | I/O | Botão BOOT / Download | Strapping pin. Deve estar HIGH durante boot normal. |
| **1** | I/O | UART0 TXD / Conector CN1 / USB-Serial | Comunicação Serial padrão com o computador. |
| **2** | Out | **TFT_DC** (Data / Command) | Strapping pin (puxado internamente para LOW no boot). |
| **3** | In | UART0 RXD / Conector CN1 / USB-Serial | Recepção Serial padrão. |
| **4** | Out | **LED RGB - Vermelho (R)** | **Ativo em nível lógico BAIXO (LOW = Aceso).** |
| **5** | Out | **MicroSD CS** | Chip Select do Cartão SD (barramento VSPI). |
| **12** | In/Out | **TFT_MISO** | Strapping pin (tensão flash VDD_SDIO). Geralmente desconectado no LCD. |
| **13** | Out | **TFT_MOSI** | Linha de dados SPI para o display. |
| **14** | Out | **TFT_SCLK** | Clock SPI para o display. |
| **15** | Out | **TFT_CS** | Chip Select do display TFT. |
| **16** | Out | **LED RGB - Verde (G)** | **Ativo em nível lógico BAIXO (LOW = Aceso).** |
| **17** | Out | **LED RGB - Azul (B)** | **Ativo em nível lógico BAIXO (LOW = Aceso).** |
| **18** | Out | **MicroSD SCK** | Clock SPI do leitor de cartão MicroSD. |
| **19** | In | **MicroSD MISO** | Linha de dados MISO do MicroSD. |
| **21** | Out | **TFT_BL** (Backlight) | Controle de iluminação da tela (HIGH = ligado; suporta PWM). |
| **22** | I/O | **Conector P3 (SDA)** | Barramento I2C externo para sensores/módulos. |
| **23** | Out | **MicroSD MOSI** | Linha de dados MOSI do MicroSD. |
| **25** | Out | **Touch XPT2046 CLK** | Clock SPI dedicado ao controlador de toque. |
| **26** | Out | **Áudio DAC / Speaker** | Saída analógica DAC ligada ao amplificador de som. |
| **27** | I/O | **Conector P3 (SCL)** | Barramento I2C externo para sensores/módulos. |
| **32** | Out | **Touch XPT2046 MOSI (DIN)** | Entrada de dados de comando do touch. |
| **33** | Out | **Touch XPT2046 CS** | Chip Select do touch controller. |
| **34** | In | **Sensor LDR (Luz)** | Entrada analógica ADC1_CH6 (Somente entrada, sem pull-up). |
| **35** | In | **Conector P3 (Extended IN)** | Entrada digital ou analógica livre (Somente entrada). |
| **36** | In | **Touch XPT2046 IRQ** | Linha de interrupção do touch (Sensor VP, somente entrada). |
| **39** | In | **Touch XPT2046 MISO (DOUT)** | Linha de saída de dados do touch (Sensor VN, somente entrada). |

---

## 2. Conectores de Expansão Onboard

### Conector P3 (Conector JST 1.25mm de 4 pinos)
Ideal para conectar sensores I2C (ex: BME280, RTC DS3231, MPU6050):
- **Pino 1**: `GND`
- **Pino 2**: `3V3`
- **Pino 3**: `GPIO 22` (SDA padrão)
- **Pino 4**: `GPIO 27` (SCL padrão)

### Conector CN1 (Porta Serial Externa)
- **Pino 1**: `GND`
- **Pino 2**: `3V3` ou `5V` (depende da revisão da placa)
- **Pino 3**: `TXD` (`GPIO 1`)
- **Pino 4**: `RXD` (`GPIO 3`)

### Conector P2 / Speaker (Alto-falante)
- Conector JST 1.25mm de 2 pinos destinado a alto-falantes pequenos de 8 ohms (0.5W a 1W).
- Acionado via `GPIO 26` através de um pequeno chip amplificador de áudio integrado (NS4168 ou PAM8302).

---

## 3. Particularidades de Barramentos SPI

A placa utiliza **três conjuntos de pinos SPI**:
1. **SPI do Display TFT**:
   - `CLK: 14`, `MOSI: 13`, `MISO: 12`, `CS: 15`, `DC: 2`
2. **SPI do Touch Screen (XPT2046)**:
   - `CLK: 25`, `MOSI: 32`, `MISO: 39`, `CS: 33`, `IRQ: 36`
3. **SPI do Slot MicroSD (VSPI padrão)**:
   - `CLK: 18`, `MOSI: 23`, `MISO: 19`, `CS: 5`

Como cada periférico possui suas próprias linhas de clock e dados, eles podem operar simultaneamente sem colisões de bus SPI, desde que cada biblioteca seja configurada com seus respectivos pinos.

---

## 4. Cuidados com Pinos Input-Only (GPI)
Os pinos **GPIO 34, 35, 36 e 39** são entradas exclusivas no silício do ESP32:
- Não possuem transistores de saída (não funcionam com `pinMode(pin, OUTPUT)` nem `digitalWrite`).
- Não possuem resistores internos de pull-up ou pull-down (`INPUT_PULLUP` não funciona neles).
- No CYD, GPIO 34 já conta com o divisor de tensão do sensor de luz LDR integrado.
