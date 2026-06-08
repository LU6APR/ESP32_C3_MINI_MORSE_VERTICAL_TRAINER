#include <Wire.h>
#include <U8x8lib.h>

/*
 *  Morse Trainer - ESP32-C3 Mini (Auto-Adaptativo + Espacio Auto 1s)
 *  by LU6APR (GP3.0)
 *  
 *  FIX: Espacio automático reducido a 1 segundo (1000ms).
 */

// ==================== PINES ====================
constexpr uint8_t PIN_DIT         = 2;
constexpr uint8_t PIN_DAH         = 3;
constexpr uint8_t PIN_RESET       = 0;
constexpr uint8_t PIN_SIDETONE    = 1;
constexpr uint8_t PIN_KEY_OUT     = 6;
constexpr uint8_t I2C_SDA         = 5;
constexpr uint8_t I2C_SCL         = 4;

// ==================== CONSTANTES ====================
constexpr uint8_t  MORSE_BUFFER_SIZE   = 32;
constexpr uint8_t  MESSAGE_BUFFER_SIZE = 64;

// Debounce (filtra rebotes mecánicos del key)
constexpr uint16_t DEBOUNCE_PRESS_MS   = 20;  
constexpr uint16_t DEBOUNCE_RELEASE_MS = 40;  

constexpr uint16_t TONE_FREQUENCY      = 800;
constexpr uint8_t  LEDC_RESOLUTION     = 8;

// Tiempo de inactividad para agregar espacio automático al final (1 segundo)
constexpr uint16_t IDLE_SPACE_TIMEOUT_MS = 1000; 

// ==================== DISPLAY ====================
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

// ==================== ESTADOS ====================
enum class DecoderState { IDLE, WAITING_SPACE };

// ==================== VARIABLES GLOBALES ====================
DecoderState currentState = DecoderState::IDLE;

unsigned long lastPulseTime      = 0;
unsigned long lastAnyInputTime   = 0;
unsigned long pulseStartTime     = 0;
unsigned long lastResetPressTime = 0;

// Variables para debounce robusto
bool lastRawDitState = HIGH;
bool lastRawDahState = HIGH;
unsigned long lastDitChangeTime = 0;
unsigned long lastDahChangeTime = 0;
bool ditStableState = HIGH;
bool dahStableState = HIGH;

// Variables para auto-adaptación (Velocidad en tiempo real)
unsigned long unitTime = 120; // Valor inicial seguro (~10-12 WPM)

char morseBuffer[MORSE_BUFFER_SIZE];
uint8_t morseBufferIndex = 0;

char decodedMessage[MESSAGE_BUFFER_SIZE];
uint8_t messageLength = 0;

bool isPulseActive    = false;
bool lastResetState   = HIGH;
bool lastPulseDisplay = false;

char lastMorseDisplay[MORSE_BUFFER_SIZE] = "";
char lastMsgDisplay[MESSAGE_BUFFER_SIZE] = "";

// ==================== TABLA MORSE ====================
const char* const morseTable[] PROGMEM = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---",
  "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",
  "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...",
  "---..", "----.",
  ".-.-.-", "--..--", "..--..", ".----.", "-.-.--",
  "-..-.", "-.--.", "-.--.-", "-.-.-.", "-....-", "-...-"
};

const char morseChars[] PROGMEM = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
  'U', 'V', 'W', 'X', 'Y', 'Z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '.', ',', '?', '\'', '!',
  '/', '(', ')', ';', '-', '='
};

constexpr uint8_t MORSE_TABLE_SIZE = sizeof(morseChars);

// ==================== PROTOTIPOS ====================
void setupPins();
void setupDisplay();
void setupTone();
void showMainScreen();
void clearScreenAndReset();
void handleDecoderInput();
void startPulse(bool isDit);
void endPulse();
void checkPulseTimeout();
void addSpaceIfIdle();
void checkResetButton();
void updateDisplay();
char decodeMorse(const char* morse);
void addMorseSymbol(char symbol);
void resetMorseBuffer();
void addToMessage(char c);
void resetMessage();
void playTone(bool state);
void controlTransmitter(bool state);
void scrollMessage(const char* fullMsg, char* display, uint8_t maxLen);
bool readDebounced(uint8_t pin, bool &lastRawState, bool &stableState, 
                   unsigned long &lastChangeTime, uint16_t debounceMs);

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println(F("\n=== Morse Trainer v2.a (Idle Space 1s) ==="));
  Serial.println(F("ESP32-C3 SuperMini | by LU6APR (GP3.0)\n"));
  
  setupPins();
  setupTone();
  setupDisplay();
  
  Serial.println(F("Caracteres: A-Z 0-9 . , ? ' ! - = / ( ) ;"));
  Serial.println(F("El sistema aprende tu velocidad automáticamente."));
  Serial.println(F("Espacio auto al dejar de pulsar (1s)."));
  Serial.println(F("Boton RESET: limpia pantalla\n"));
  
  showMainScreen();
  lastAnyInputTime = millis();
}

void setupPins() {
  pinMode(PIN_DIT, INPUT_PULLUP);
  pinMode(PIN_DAH, INPUT_PULLUP);
  pinMode(PIN_RESET, INPUT_PULLUP);
  pinMode(PIN_SIDETONE, OUTPUT);
  pinMode(PIN_KEY_OUT, OUTPUT);
  
  controlTransmitter(false);
  digitalWrite(PIN_SIDETONE, LOW);
  
  ditStableState = digitalRead(PIN_DIT);
  dahStableState = digitalRead(PIN_DAH);
  lastRawDitState = ditStableState;
  lastRawDahState = dahStableState;
}

void setupTone() {
  ledcAttach(PIN_SIDETONE, TONE_FREQUENCY, LEDC_RESOLUTION);
  ledcWrite(PIN_SIDETONE, 0);
}

void setupDisplay() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setPowerSave(0);
  u8x8.clearDisplay();
}

void showMainScreen() {
  u8x8.clearDisplay();
  u8x8.drawString(1, 0, ">TRAINER<");
  u8x8.drawString(1, 2, "               ");
  u8x8.drawString(1, 4, "Msg:");
  u8x8.drawString(1, 5, "               ");
  
  lastMorseDisplay[0] = '\0';
  lastMsgDisplay[0] = '\0';
}

void clearScreenAndReset() {
  if (isPulseActive) {
    playTone(false);
    controlTransmitter(false);
    isPulseActive = false;
  }
  
  currentState = DecoderState::IDLE;
  lastPulseTime = 0;
  pulseStartTime = 0;
  
  resetMorseBuffer();
  resetMessage();
  
  lastMorseDisplay[0] = '\0';
  lastMsgDisplay[0] = '\0';
  lastPulseDisplay = false;
  
  showMainScreen();
  lastAnyInputTime = millis();
  
  Serial.println(F("[RESET] Pantalla limpiada"));
}

// ==================== DEBOUNCE ROBUSTO ====================
bool readDebounced(uint8_t pin, bool &lastRawState, bool &stableState, 
                   unsigned long &lastChangeTime, uint16_t debounceMs) {
  bool rawState = digitalRead(pin);
  unsigned long currentTime = millis();
  
  if (rawState != lastRawState) {
    lastChangeTime = currentTime;
    lastRawState = rawState;
  }
  
  if ((currentTime - lastChangeTime) > debounceMs) {
    if (rawState != stableState) {
      stableState = rawState;
      return true;
    }
  }
  return false;
}

// ==================== DECODER ====================
void handleDecoderInput() {
  unsigned long currentTime = millis();
  
  bool ditChanged = readDebounced(PIN_DIT, lastRawDitState, ditStableState, 
                                   lastDitChangeTime, DEBOUNCE_PRESS_MS);
  bool dahChanged = readDebounced(PIN_DAH, lastRawDahState, dahStableState, 
                                   lastDahChangeTime, DEBOUNCE_PRESS_MS);
  
  if (!isPulseActive) {
    if (ditStableState == LOW && ditChanged) {
      startPulse(true);
      lastAnyInputTime = currentTime;
    }
    else if (dahStableState == LOW && dahChanged) {
      startPulse(false);
      lastAnyInputTime = currentTime;
    }
  }
  else if (ditStableState == HIGH && dahStableState == HIGH) {
    if ((currentTime - lastDitChangeTime) > DEBOUNCE_RELEASE_MS &&
        (currentTime - lastDahChangeTime) > DEBOUNCE_RELEASE_MS) {
      endPulse();
      lastAnyInputTime = currentTime;
    }
  }
}

void startPulse(bool isDit) {
  isPulseActive = true;
  pulseStartTime = millis();
  playTone(true);
  controlTransmitter(true);
  
  Serial.print(isDit ? F("[DIT]") : F("[DAH]"));
}

void endPulse() {
  unsigned long duration = millis() - pulseStartTime;
  isPulseActive = false;
  playTone(false);
  controlTransmitter(false);
  
  Serial.printf(" (%lums) ", duration);
  
  if (duration < unitTime * 2) {
    addMorseSymbol('.');
    Serial.print('.');
    unitTime = (unitTime * 3 + duration) / 4;
  } else {
    addMorseSymbol('-');
    Serial.print('-');
    unitTime = (unitTime * 3 + (duration / 3)) / 4;
  }
  
  if (unitTime < 50) unitTime = 50;
  if (unitTime > 400) unitTime = 400;
  
  lastPulseTime = millis();
  currentState = DecoderState::WAITING_SPACE;
}

// ==================== TIMEOUTS ====================
void checkPulseTimeout() {
  if (currentState != DecoderState::WAITING_SPACE || isPulseActive) {
    return;
  }
  
  unsigned long idleTime = millis() - lastPulseTime;
  
  if (idleTime > unitTime * 7) {
    if (messageLength > 0 && decodedMessage[messageLength - 1] != ' ') {
      addToMessage(' ');
      Serial.print(F(" / "));
    }
    resetMorseBuffer();
    currentState = DecoderState::IDLE;
  }
  else if (idleTime > unitTime * 3) {
    char decoded = decodeMorse(morseBuffer);
    if (decoded != '?') {
      addToMessage(decoded);
      Serial.print(' ');
      Serial.print(decoded);
      Serial.print(' ');
    } else if (morseBufferIndex > 0) {
      Serial.printf("[?%s]", morseBuffer);
    }
    resetMorseBuffer();
    currentState = DecoderState::IDLE;
  }
}

// ==================== ESPACIO POR INACTIVIDAD ====================
void addSpaceIfIdle() {
  if (isPulseActive || messageLength == 0) {
    return;
  }
  
  unsigned long idleTime = millis() - lastAnyInputTime;
  
  // Si pasa 1 segundo sin tocar el key, agrega un espacio final
  if (idleTime >= IDLE_SPACE_TIMEOUT_MS) {
    if (decodedMessage[messageLength - 1] != ' ') {
      addToMessage(' ');
      Serial.println(F("\n--- ESPACIO AUTO (Inactividad 1s) ---"));
    }
  }
}

// ==================== BUFFER Y MENSAJE ====================
void addMorseSymbol(char symbol) {
  if (morseBufferIndex < MORSE_BUFFER_SIZE - 1) {
    morseBuffer[morseBufferIndex++] = symbol;
    morseBuffer[morseBufferIndex] = '\0';
  }
}

void resetMorseBuffer() {
  morseBuffer[0] = '\0';
  morseBufferIndex = 0;
}

void addToMessage(char c) {
  if (messageLength >= MESSAGE_BUFFER_SIZE - 1) {
    for (uint8_t i = 0; i < messageLength - 1; i++) {
      decodedMessage[i] = decodedMessage[i + 1];
    }
    decodedMessage[messageLength - 1] = c;
  } else {
    decodedMessage[messageLength++] = c;
    decodedMessage[messageLength] = '\0';
  }
}

void resetMessage() {
  decodedMessage[0] = '\0';
  messageLength = 0;
}

char decodeMorse(const char* morse) {
  if (morse[0] == '\0') return '?';
  
  for (uint8_t i = 0; i < MORSE_TABLE_SIZE; i++) {
    char tableEntry[10];
    strcpy_P(tableEntry, (char*)pgm_read_ptr(&morseTable[i]));
    
    if (strcmp(morse, tableEntry) == 0) {
      return pgm_read_byte(&morseChars[i]);
    }
  }
  return '?';
}

// ==================== SALIDAS Y UI ====================
void playTone(bool state) {
  if (state) {
    ledcWrite(PIN_SIDETONE, 127);
  } else {
    ledcWrite(PIN_SIDETONE, 0);
  }
}

void controlTransmitter(bool state) {
  digitalWrite(PIN_KEY_OUT, state ? HIGH : LOW);
}

void checkResetButton() {
  int resetState = digitalRead(PIN_RESET);
  unsigned long currentTime = millis();
  
  if (resetState == LOW && lastResetState == HIGH) {
    if (currentTime - lastResetPressTime > DEBOUNCE_PRESS_MS) {
      clearScreenAndReset();
      lastResetPressTime = currentTime;
    }
  }
  lastResetState = resetState;
}

void scrollMessage(const char* fullMsg, char* display, uint8_t maxLen) {
  uint8_t len = strlen(fullMsg);
  if (len <= maxLen) {
    strcpy(display, fullMsg);
  } else {
    strcpy(display, "...");
    strncat(display, fullMsg + len - (maxLen - 3), maxLen - 3);
  }
}

void updateDisplay() {
  char currentMorse[MORSE_BUFFER_SIZE];
  char currentMsg[MESSAGE_BUFFER_SIZE];
  
  scrollMessage(morseBuffer, currentMorse, 14);
  scrollMessage(decodedMessage, currentMsg, 14);
  
  if (strcmp(currentMorse, lastMorseDisplay) != 0) {
    char paddedMorse[16] = "               ";
    strncpy(paddedMorse, currentMorse, strlen(currentMorse));
    u8x8.drawString(1, 2, paddedMorse);
    strcpy(lastMorseDisplay, currentMorse);
  }
  
  if (strcmp(currentMsg, lastMsgDisplay) != 0) {
    char paddedMsg[16] = "               ";
    strncpy(paddedMsg, currentMsg, strlen(currentMsg));
    u8x8.drawString(1, 5, paddedMsg);
    strcpy(lastMsgDisplay, currentMsg);
  }
  
  if (isPulseActive != lastPulseDisplay) {
    u8x8.drawString(14, 0, isPulseActive ? "*" : " ");
    lastPulseDisplay = isPulseActive;
  }
}

// ==================== LOOP ====================
void loop() {
  checkResetButton();
  handleDecoderInput();
  checkPulseTimeout();
  addSpaceIfIdle(); 
  updateDisplay();
}