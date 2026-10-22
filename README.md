# CYD 2.8 - Painel Digital & Receptor de Imagens BLE (v8.2)

Sistema completo de letreiro digital rolante em alta velocidade e receptor de fotos personalizadas para a placa **ESP32-2432S028 ("CYD - Cheap Yellow Display")**, controlado via Bluetooth Low Energy (BLE) através de uma aplicação web responsiva executada diretamente no navegador Google Chrome (Android ou Computador).

---

## 📋 Sumário
1. [Recursos Principais](#-recursos-principais)
2. [Hardware Utilizado](#-hardware-utilizado)
3. [Bibliotecas Necessárias no Arduino IDE](#-bibliotecas-necessárias-no-arduino-ide)
4. [Configuração da Placa no Arduino IDE](#-configuração-da-placa-no-arduino-ide)
5. [Como Gravar o Firmware](#-como-gravar-o-firmware)
6. [Como Usar o Painel Web (Celular / PC)](#-como-usar-o-painel-web-celular--pc)
7. [Protocolo de Comunicação BLE](#-protocolo-de-comunicação-ble)
8. [Estrutura dos Arquivos](#-estrutura-dos-arquivos)
9. [Dicas Importantes para o Dia do Evento](#-dicas-importantes-para-o-dia-do-evento)

---

## 🌟 Recursos Principais

- **Letreiro Rolante Gigante (70% da Tela)**:
  - As letras podem alcançar até **~168 pixels de altura** na tela física de 240 pixels.
  - Rolagem ultrafluida a ~55 FPS sem oscilações (*flicker-free*) utilizando arquitetura *Dual-Half* em 16 bits.
- **Tipografia e Acentuação 100% em Português**:
  - Renderização oficial da biblioteca **U8g2** (família *FreeUniversal Bold*).
  - Suporte completo a **til (`ã`, `õ`, `Ã`, `Õ`)**, **cedilha (`ç`, `Ç`)**, acentos circunflexos (`ê`, `â`, `ô`, `Ê`), agudos (`é`, `á`, `í`, `ó`, `ú`) e pontuações especiais.
- **Cores Nativas Puras de 16 Bits (RGB565)**:
  - Seleção livre de qualquer cor de texto e fundo via Color Picker na interface web.
  - Zero distorção cromática, sem faixas azuis ou bordas irregulares.
- **Transmissão de Fotos com Recorte Fino**:
  - Enquadre qualquer foto da galeria do celular arrastando o dedo e aplicando zoom no visor 320x240 antes do envio.
  - Compressão e decodificação JPEG direta no ESP32.
- **Arquitetura de Memória Protegida (Anti-Bootloop)**:
  - Alocação dinâmica de buffer de imagem e sprite otimizado de apenas 57 KB de RAM, garantindo estabilidade absoluta da pilha BLE do ESP32.

---

## 🖥️ Hardware Utilizado

- **Placa**: ESP32-2432S028 ("Cheap Yellow Display" / CYD)
- **Tela**: TFT Colorido de 2.8" (320x240 pixels, controlador ILI9341 / ST7789)
- **Comunicação**: Bluetooth Low Energy (BLE 4.2+)
- **Alimentação**: Cabo Micro-USB ou USB Tipo-C (5V)

### Pinout Crítico Onboard:
| Periférico | Pino GPIO | Observação |
| :--- | :--- | :--- |
| **Backlight LCD Principal** | `GPIO 21` | Ativado em `HIGH` (Obrigatório para acender a tela) |
| **Backlight LCD Alternativo** | `GPIO 27` | Ativado em `HIGH` (Compatibilidade entre revisões da placa) |
| **LED RGB - Vermelho** | `GPIO 4` | Ativo em `LOW` |
| **LED RGB - Verde** | `GPIO 16` | Ativo em `LOW` |
| **LED RGB - Azul** | `GPIO 17` | Ativo em `LOW` |

---

## 📦 Bibliotecas Necessárias no Arduino IDE

Certifique-se de ter as seguintes bibliotecas instaladas no seu Arduino IDE:

1. **`TFT_eSPI`** (por Bodmer)
   - Instale pelo *Gerenciador de Bibliotecas* do Arduino IDE.
   - **Atenção**: O arquivo `User_Setup.h` da biblioteca deve estar configurado para a placa CYD (driver `ILI9341_2_DRIVER` com pinos MOSI=13, SCLK=14, CS=15, DC=2).
2. **`TJpg_Decoder`** (por Bodmer)
   - Instale pelo *Gerenciador de Bibliotecas* do Arduino IDE.
3. **`U8g2_for_TFT_eSPI`** (por Bodmer / olikraus)
   - Como não consta no buscador padrão do IDE, utilize o arquivo ZIP já disponibilizado na raiz deste projeto:
     - No menu do Arduino IDE: `Sketch` -> `Incluir Biblioteca` -> `Adicionar Biblioteca .ZIP...`
     - Selecione o arquivo: `U8g2_for_TFT_eSPI-master.zip`.
4. **`BLEDevice`** (Nativa do core `esp32 by Espressif Systems`).

---

## ⚙️ Configuração da Placa no Arduino IDE

No menu **Ferramentas (Tools)** do Arduino IDE, defina os seguintes parâmetros:

- **Placa (Board)**: `ESP32 Dev Module`
- **CPU Frequency**: `240MHz (WiFi/BT)`
- **Flash Frequency**: `80MHz`
- **Flash Mode**: `QIO`
- **Partition Scheme**: `Huge APP (3MB No OTA/1MB SPIFFS)` *(Recomendado para garantir espaço na memória de programa)*
- **Upload Speed**: `921600` (se der erro de comunicação durante o upload, reduza para `115200`)
- **Porta**: Selecione a porta COM correspondente ao seu cabo USB conectado.

---

## 🚀 Como Gravar o Firmware

1. Abra o arquivo [`teste/teste.ino`](teste/teste.ino) (ou [`cyd_bluetooth_marquee/cyd_bluetooth_marquee.ino`](cyd_bluetooth_marquee/cyd_bluetooth_marquee.ino)) no Arduino IDE.
2. Conecte a placa ESP32 CYD ao computador pelo cabo USB.
3. Clique no botão **Carregar (Upload / Seta para a direita)**.
4. Aguarde a compilação e a gravação. Ao finalizar, a tela acenderá imediatamente exibindo o letreiro padrão inicial e o LED onboard ficará azul indicando que o Bluetooth está aguardando conexão.

---

## 📱 Como Usar o Painel Web (Celular / PC)

O painel de controle remoto é um arquivo HTML/JavaScript autônomo localizado em [`web_remote/index.html`](web_remote/index.html). Ele **não requer internet** e se comunica diretamente com a placa via **Web Bluetooth**.

### Passo a Passo no Celular (Android):
1. Transfira o arquivo [`web_remote/index.html`](web_remote/index.html) para o seu celular (ou hospede em qualquer servidor local/GitHub Pages).
2. Abra o arquivo utilizando o navegador **Google Chrome**.
3. Toque no botão **"Conectar ao CYD (Bluetooth)"**.
4. Na janela de dispositivos próximos, selecione **CYD-Letreiro** e toque em **Parear**.
5. O status mudará para verde: **● Conectado ao CYD**.

### Controles Disponíveis no Painel:
- **Texto da Frase**: Digite qualquer mensagem de até 140 caracteres, inclusive com acentos (`ã`, `ç`, `é`, `ô`, etc.).
- **Tamanho da Letra**:
  - `Normal` (~40px)
  - `Médio` (~80px)
  - `Grande` (~120px)
  - `GIGANTE (75% Tela)` (~168px de altura!)
- **Velocidade de Rolagem**: Slider ajustável de 1 (lento e suave) a 15 (rápido).
- **Color Pickers**: Escolha qualquer cor para a letra e qualquer cor para o fundo da tela. Há botões de temas rápidos (Preto/Amarelo, Preto/Branco, etc.).
- **Modo de Layout**:
  - *Modo Minimalista*: Tela cheia limpa, apenas o letreiro gigante rolando no centro.
  - *Modo Completo*: Inclui cabeçalho superior e barra inferior com status do Bluetooth.
- **Botão "Atualizar Letreiro na Tela"**: Envia todas as configurações atômicas de uma só vez para o display.
- **Envio de Fotos (Recorte Fino)**:
  1. Toque em "Escolha uma imagem da galeria".
  2. Arraste a imagem no visor preto para posicioná-la e use o slider de Zoom.
  3. Toque em **"Enviar Foto para a Tela"**. A foto será enviada progressivamente e exibida na tela.
  4. Para voltar ao letreiro, basta tocar em **"Voltar ao Letreiro de Texto"**.

---

## 📡 Protocolo de Comunicação BLE

- **Nome do Dispositivo**: `CYD-Letreiro`
- **Service UUID**: `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
- **RX Characteristic (Gravação)**: `6E400002-B5A3-F393-E0A9-E50E24DCCA9E`
- **TX Characteristic (Notificação)**: `6E400003-B5A3-F393-E0A9-E50E24DCCA9E`

### Comandos de Texto (Atômico $CFG):
Formato: `$CFG:<COR_TEXTO_HEX>,<COR_FUNDO_HEX>,<TAMANHO>,<VELOCIDADE>,<LAYOUT>,<TEXTO_UTF8>`
*Exemplo*: `$CFG:FFE600,000000,4,4,1,CIÊNCIA! Atenção ao café!`

---

## 📁 Estrutura dos Arquivos

```text
arduino/
│
├── README.md                      # Este manual completo de instruções
├── U8g2_for_TFT_eSPI-master.zip   # Arquivo da biblioteca de fontes U8g2
│
├── teste/
│   └── teste.ino                  # Código-fonte principal do firmware (v8.2)
│
├── cyd_bluetooth_marquee/
│   └── cyd_bluetooth_marquee.ino  # Cópia idêntica de segurança do firmware
│
└── web_remote/
    └── index.html                 # Interface Web Bluetooth para Android / Chrome
```

---

## 💡 Dicas Importantes para o Dia do Evento

1. **Alimentação Estável**:
   - Utilize um carregador de celular de boa qualidade (5V / 2A) conectado à porta USB da placa. Fontes com baixa amperagem ou portas USB de computadores antigos podem reiniciar o ESP32 ao acionar o transmissor Bluetooth.
2. **Navegador no Celular**:
   - O recurso de **Web Bluetooth** funciona exclusivamente no **Google Chrome** no Android (ou no Chrome em PCs com Bluetooth). O navegador Safari (iOS) não suporta Web Bluetooth nativamente.
3. **Reconexão Rápida**:
   - Caso o celular saia do raio de alcance do Bluetooth, a placa continua rodando o letreiro normalmente. Ao voltar, basta abrir a página e tocar em "Conectar ao CYD".
4. **Reiniciar a Placa**:
   - Caso precise resetar a placa durante o evento, dê um único clique no botão físico **RST / EN** na lateral da placa. Ela reinicializará em menos de 2 segundos.
