#include <Wire.h>
#include <U8x8lib.h>

/*
 *  Morse Trainer Minimalista
 *  by LU6APR (GP3.0)
 *  para ESP32-C3 mini con display SSD1106
 *  
 *  Caracteres soportados: A-Z, 0-9, ., ,, ?, ', !, /, (, ), ;
 *  Espacio automatico: 1 segundo sin entrada
 *  Modo: TRAINER (solo decodificación)
 *  Salida para transmisor: PIN_KEY_OUT
 *  Tono de monitoreo: 800Hz
 *  Boton: limpia pantalla y resetea
 */

// Definición de pines
#define PIN_DIT         2      // Entrada pulso corto
#define PIN_DAH         3      // Entrada pulso largo  
#define PIN_RESET       0      // Botón para limpiar pantalla (antes COMMAND)
#define PIN_SIDETONE    1      // Salida tono monitoreo
#define PIN_KEY_OUT     6      // Salida para controlar transmisor

#define I2C_SDA         5
#define I2C_SCL         4

// Configuración del display SSD1106
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

// Configuración del trainer
#define MORSE_UNIT_MS    100   // Duración de la unidad base en ms
#define DECODE_BUFFER    64
#define SPACE_TIMEOUT    1000  // 1 segundo sin entrada = espacio

// Variables globales
enum DecoderState { IDLE, RECEIVING_DIT, RECEIVING_DAH, WAITING_SPACE };

DecoderState currentState = IDLE;
unsigned long lastPulseTime = 0;
unsigned long lastAnyInputTime = 0;
unsigned long lastSpaceAddedTime = 0;
unsigned long pulseStartTime = 0;
char morseBuffer[DECODE_BUFFER];
int morseBufferIndex = 0;
char decodedChar;
String decodedMessage = "";
bool isPulseActive = false;
int lastResetState = HIGH;

// Variables para display sin parpadeo
String lastMorseStr = "";
String lastMsgStr = "";
bool lastPulseState = false;

// Tabla Morse completa
const char* morseTable[] = {
  // Letras A-Z
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",
  // Números 0-9
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...",
  "---..", "----.",
  // Puntuación
  ".-.-.-", "--..--", "..--..", ".----.", "-.-.--",
  // Especiales
  "-..-.",   // /
  "-.--.",   // (
  "-.--.-",  // )
  "-.-.-."   // ;
};

const char morseChars[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
  'U', 'V', 'W', 'X', 'Y', 'Z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '.', ',', '?', '\'', '!',
  '/', '(', ')', ';'
};

// Prototipos
char decodeMorse(const char* morse);
void addMorseSymbol(char symbol);
void resetMorseBuffer();
void updateDisplay();
void playTone(int state);
void controlTransmitter(bool state);
void checkResetButton();
void addSpaceIfIdle();
void handleDecoderInput();
void startPulse(bool isDit);
void endPulse();
void checkPulseTimeout();
void initDisplaySSD1106();
void showSplashScreen();
void clearScreenAndReset();

void setup() {
  Serial.begin(115200);
  
  // Configurar pines
  pinMode(PIN_DIT, INPUT_PULLUP);
  pinMode(PIN_DAH, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);
  pinMode(PIN_SIDETONE, OUTPUT);
  pinMode(PIN_KEY_OUT, OUTPUT);
  
  controlTransmitter(false);
  
  // Inicializar I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000);
  
  delay(100);
  
  // Inicialización específica para SSD1106
  initDisplaySSD1106();
  
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setPowerSave(0);
  
  // Limpieza inicial completa
  u8x8.clearDisplay();
  for(int i = 0; i < 8; i++) {
    u8x8.drawString(0, i, "                ");
  }
  u8x8.clearDisplay();
  
  memset(morseBuffer, 0, DECODE_BUFFER);
  lastAnyInputTime = millis();
  lastSpaceAddedTime = millis();
  
  // Mostrar pantalla de bienvenida
  showSplashScreen();
  
  // Pantalla principal
  u8x8.clearDisplay();
  u8x8.drawString(1, 0, ">TRAINER<");
  u8x8.drawString(1, 4, "Msg:");
  
  lastMorseStr = "";
  lastMsgStr = "";
  lastPulseState = false;
  
  Serial.println("Morse Trainer iniciado - FINAL");
  Serial.println("Caracteres: A-Z 0-9 . , ? ' ! / ( ) ;");
  Serial.println("Credits: LU6APR (GP3.0)");
  Serial.println("Boton: limpia pantalla");
}

void showSplashScreen() {
  // Pantalla 1 - Logo y créditos
  u8x8.clearDisplay();
  u8x8.drawString(4, 1, "MORSE");
  u8x8.drawString(3, 2, "TRAINER");
  u8x8.drawString(6, 4, "v1.0");
  u8x8.drawString(3, 6, "by LU6APR");
  u8x8.drawString(4, 7, "(GP3.0)");
  delay(3000);
  
  // Pantalla 2 - Información
  u8x8.clearDisplay();
  u8x8.drawString(2, 2, "MINIMALIST");
  u8x8.drawString(4, 3, "MORSE");
  u8x8.drawString(3, 4, "TRAINER");
  u8x8.drawString(2, 6, "for ESP32-C3");
  delay(2500);
  
  // Pantalla 3 - Caracteres soportados
  u8x8.clearDisplay();
  u8x8.drawString(1, 1, "A-Z 0-9");
  u8x8.drawString(1, 3, ". , ? ' !");
  u8x8.drawString(1, 5, "/ ( ) ;");
  u8x8.drawString(0, 7, "1s AUTO SPACE");
  delay(2500);
}

void clearScreenAndReset() {
  // Desactivar cualquier pulso activo
  if (isPulseActive) {
    playTone(0);
    controlTransmitter(false);
    isPulseActive = false;
  }
  
  // Resetear estado del decoder
  currentState = IDLE;
  lastPulseTime = 0;
  pulseStartTime = 0;
  
  // Limpiar buffer de Morse
  resetMorseBuffer();
  
  // Limpiar mensaje decodificado
  decodedMessage = "";
  
  // Limpiar variables de display
  lastMorseStr = "";
  lastMsgStr = "";
  lastPulseState = false;
  
  // Limpiar pantalla completamente
  u8x8.clearDisplay();
  
  // Dibujar pantalla principal limpia
  u8x8.drawString(1, 0, ">TRAINER<");
  u8x8.drawString(1, 4, "Msg:");
  
  // Limpiar líneas de código Morse y mensaje
  u8x8.drawString(1, 2, "               ");
  u8x8.drawString(1, 5, "               ");
  
  // Resetear timer de inactividad
  lastAnyInputTime = millis();
  lastSpaceAddedTime = millis();
  
  Serial.println("Pantalla limpiada por boton RESET");
}

void initDisplaySSD1106() {
  // Comandos específicos para SSD1106
  Wire.beginTransmission(0x3C);
  Wire.write(0x00); // Modo comando
  Wire.write(0xAE); // Display OFF
  Wire.write(0xD5); // Reloj
  Wire.write(0x80);
  Wire.write(0xA8); // Multiplex
  Wire.write(0x3F);
  Wire.write(0xD3); // Display offset
  Wire.write(0x00); // Offset 0
  Wire.write(0x40); // Línea de inicio 0
  Wire.write(0x8D); // Charge pump
  Wire.write(0x14); // Habilitar
  Wire.write(0x20); // Modo memoria
  Wire.write(0x00); // Horizontal
  Wire.write(0xA0); // Segment remap NORMAL
  Wire.write(0xC0); // COM scan direction NORMAL
  Wire.write(0xDA); // Pines COM
  Wire.write(0x12);
  Wire.write(0x81); // Contraste
  Wire.write(0xCF);
  Wire.write(0xD9); // Pre-carga
  Wire.write(0xF1);
  Wire.write(0xDB); // VCOM
  Wire.write(0x40);
  Wire.write(0xA4); // Restaurar
  Wire.write(0xA6); // Normal
  Wire.write(0x2E); // Scroll OFF
  Wire.write(0xAF); // Display ON
  Wire.endTransmission();
  delay(100);
  
  // Limpiar toda la pantalla
  for(int page = 0; page < 8; page++) {
    Wire.beginTransmission(0x3C);
    Wire.write(0x00);
    Wire.write(0xB0 | page);
    Wire.write(0x00);
    Wire.write(0x10);
    Wire.endTransmission();
    
    Wire.beginTransmission(0x3C);
    Wire.write(0x40);
    for(int col = 0; col < 128; col++) {
      Wire.write(0x00);
    }
    Wire.endTransmission();
  }
}

void loop() {
  // Verificar botón de reset
  checkResetButton();
  
  // Manejar entrada del decoder
  handleDecoderInput();
  
  // Verificar timeouts
  checkPulseTimeout();
  addSpaceIfIdle();
  
  // Actualizar display
  updateDisplay();
  
  delay(5);
}

void handleDecoderInput() {
  bool ditState = digitalRead(PIN_DIT);
  bool dahState = digitalRead(PIN_DAH);
  
  // Detectar inicio de pulso DIT
  if (ditState == LOW && !isPulseActive) {
    startPulse(true);
    lastAnyInputTime = millis();
  }
  
  // Detectar inicio de pulso DAH
  if (dahState == LOW && !isPulseActive) {
    startPulse(false);
    lastAnyInputTime = millis();
  }
  
  // Detectar fin de pulso (ambos pines en HIGH)
  if ((ditState == HIGH && dahState == HIGH) && isPulseActive) {
    endPulse();
    lastAnyInputTime = millis();
  }
}

void startPulse(bool isDit) {
  isPulseActive = true;
  pulseStartTime = millis();
  playTone(1);
  controlTransmitter(true);
}

void endPulse() {
  unsigned long pulseDuration = millis() - pulseStartTime;
  isPulseActive = false;
  playTone(0);
  controlTransmitter(false);
  
  // Determinar si fue DIT o DAH basado en duración
  if (pulseDuration < MORSE_UNIT_MS * 1.5) {
    addMorseSymbol('.');
    Serial.print(".");
  } else if (pulseDuration < MORSE_UNIT_MS * 3.5) {
    addMorseSymbol('-');
    Serial.print("-");
  }
  
  lastPulseTime = millis();
  currentState = WAITING_SPACE;
}

void addMorseSymbol(char symbol) {
  if (morseBufferIndex < DECODE_BUFFER - 1) {
    morseBuffer[morseBufferIndex++] = symbol;
    morseBuffer[morseBufferIndex] = '\0';
  }
}

void checkPulseTimeout() {
  if (currentState == WAITING_SPACE && !isPulseActive) {
    unsigned long idleTime = millis() - lastPulseTime;
    
    // Espacio entre letras (3 unidades)
    if (idleTime > MORSE_UNIT_MS * 3) {
      decodedChar = decodeMorse(morseBuffer);
      if (decodedChar != '?') {
        decodedMessage += decodedChar;
        Serial.print(" | ");
        Serial.print(decodedChar);
        Serial.print(" | ");
      }
      resetMorseBuffer();
      currentState = IDLE;
    }
    // Espacio entre palabras (7 unidades)
    else if (idleTime > MORSE_UNIT_MS * 7) {
      if (decodedMessage.length() > 0 && decodedMessage.charAt(decodedMessage.length()-1) != ' ') {
        decodedMessage += ' ';
        Serial.print(" / ");
      }
      resetMorseBuffer();
      currentState = IDLE;
    }
  }
}

void addSpaceIfIdle() {
  unsigned long idleTime = millis() - lastAnyInputTime;
  
  // Agregar espacio después de 1 segundo sin entrada
  if (!isPulseActive && 
      decodedMessage.length() > 0 &&
      idleTime >= SPACE_TIMEOUT &&
      (millis() - lastSpaceAddedTime > SPACE_TIMEOUT)) {
    
    if (decodedMessage.charAt(decodedMessage.length() - 1) != ' ') {
      decodedMessage += ' ';
      Serial.println(" --- ESPACIO AUTO (1s) --- ");
      lastSpaceAddedTime = millis();
    }
  }
}

char decodeMorse(const char* morse) {
  if (strlen(morse) == 0) return '?';
  
  for (int i = 0; i < (int)(sizeof(morseChars)); i++) {
    if (strcmp(morse, morseTable[i]) == 0) {
      return morseChars[i];
    }
  }
  return '?';
}

void resetMorseBuffer() {
  memset(morseBuffer, 0, DECODE_BUFFER);
  morseBufferIndex = 0;
}

void playTone(int state) {
  if (state == 1) {
    tone(PIN_SIDETONE, 800);
  } else {
    noTone(PIN_SIDETONE);
  }
}

void controlTransmitter(bool state) {
  digitalWrite(PIN_KEY_OUT, state ? HIGH : LOW);
}

void checkResetButton() {
  int resetState = digitalRead(PIN_RESET);
  
  // Detectar flanco descendente (botón presionado)
  if (resetState == LOW && lastResetState == HIGH) {
    clearScreenAndReset();
    delay(300); // Debounce
  }
  
  lastResetState = resetState;
}

void updateDisplay() {
  String currentMorseStr = "";
  if (morseBufferIndex > 0) {
    currentMorseStr = String(morseBuffer);
    if (currentMorseStr.length() > 14) {
      currentMorseStr = "..." + currentMorseStr.substring(currentMorseStr.length() - 11);
    }
  }
  
  String currentMsgStr = decodedMessage;
  if (currentMsgStr.length() > 14) {
    currentMsgStr = "..." + currentMsgStr.substring(currentMsgStr.length() - 11);
  }
  
  // Actualizar código Morse (fila 2)
  if (currentMorseStr != lastMorseStr) {
    u8x8.drawString(1, 2, "               ");
    u8x8.drawString(1, 2, currentMorseStr.c_str());
    lastMorseStr = currentMorseStr;
  }
  
  // Actualizar mensaje (fila 5)
  if (currentMsgStr != lastMsgStr) {
    u8x8.drawString(1, 5, "               ");
    u8x8.drawString(1, 5, currentMsgStr.c_str());
    lastMsgStr = currentMsgStr;
  }
  
  // Actualizar indicador de pulso (fila 0, columna 14)
  if (isPulseActive != lastPulseState) {
    if (isPulseActive) {
      u8x8.drawString(14, 0, "*");
    } else {
      u8x8.drawString(14, 0, " ");
    }
    lastPulseState = isPulseActive;
  }
}