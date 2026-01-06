/**
 * @file main.cpp
 * @brief Phase 4: Kombinierter Test (Taster → LED)
 * @version 1.0.0
 * 
 * Taster drücken → entsprechende LED leuchtet
 * Taster 1 → LED 1, Taster 2 → LED 2, ...
 * 
 * Hardware:
 * - 2x CD4021B (Taster, Active-Low)
 * - 2x 74HC595 (LED, Active-High)
 * - Shared SPI-Bus (SCK gemeinsam)
 */

#include <Arduino.h>
#include "config.h"

// =============================================================================
// Zustand
// =============================================================================

static uint8_t btnRaw[BTN_BYTES];
static uint8_t btnLast[BTN_BYTES];
static uint8_t ledState[LED_BYTES];

// =============================================================================
// Low-Level: Clock
// =============================================================================

static inline void clockPulse() {
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(CLOCK_DELAY_US);
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(CLOCK_DELAY_US);
}

// =============================================================================
// CD4021B: Taster lesen
// =============================================================================

static void readButtons() {
    // Parallel Load
    // Warum? Alle 8 Eingänge gleichzeitig in Shift-Register übernehmen
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(2);
    
    // Serial Shift (erst lesen, dann clocken)
    // Warum? Q8 liegt nach Load sofort am Ausgang
    for (size_t ic = 0; ic < BTN_BYTES; ++ic) {
        uint8_t byte = 0;
        for (int8_t bit = 7; bit >= 0; --bit) {
            if (digitalRead(PIN_BTN_MISO)) {
                byte |= (1 << bit);
            }
            clockPulse();
        }
        btnRaw[ic] = byte;
    }
}

// =============================================================================
// 74HC595: LEDs schreiben
// =============================================================================

static void writeLEDs() {
    // Warum rückwärts? Daisy-Chain: Letztes IC zuerst schieben
    for (int16_t ic = LED_BYTES - 1; ic >= 0; --ic) {
        uint8_t byte = ledState[ic];
        for (int8_t bit = 7; bit >= 0; --bit) {
            digitalWrite(PIN_LED_MOSI, (byte >> bit) & 1);
            clockPulse();
        }
    }
    
    // Latch: Daten in Ausgaberegister übernehmen
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_LED_RCK, LOW);
}

// =============================================================================
// PWM
// =============================================================================

static void setLEDBrightness(uint8_t percent) {
    // OE ist Active-LOW: duty 0 = voll an, duty 255 = aus
    uint8_t duty = 255 - (percent * 255 / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

// =============================================================================
// API: Taster
// =============================================================================

static bool isPressed(uint8_t id) {
    if (id < 1 || id > BTN_COUNT) return false;
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = 7 - ((id - 1) % 8);  // Bit7=T1 (Hardware-bedingt)
    // Active-Low: Bit=0 bedeutet gedrückt
    return !(btnRaw[ic] & (1 << bit));
}

// =============================================================================
// API: LED
// =============================================================================

static void setLED(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT) return;
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = (id - 1) % 8;  // Bit0=L1 (Hardware-bedingt)
    // Active-High: Bit=1 bedeutet an
    if (on) {
        ledState[ic] |= (1 << bit);
    } else {
        ledState[ic] &= ~(1 << bit);
    }
}

static void clearLEDs() {
    for (size_t i = 0; i < LED_BYTES; ++i) {
        ledState[i] = 0x00;
    }
}

// =============================================================================
// Hilfsfunktionen
// =============================================================================

static bool btnChanged() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnRaw[i] != btnLast[i]) return true;
    }
    return false;
}

static void btnSave() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnLast[i] = btnRaw[i];
    }
}

static bool anyPressed() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnRaw[i] != 0xFF) return true;
    }
    return false;
}

// =============================================================================
// Debug-Ausgabe
// =============================================================================

static void printBinary(uint8_t* arr, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) {
        if (i > 0) Serial.print(" ");
        for (int8_t bit = 7; bit >= 0; --bit) {
            Serial.print((arr[i] >> bit) & 1);
        }
    }
}

static void printState() {
    // Aktive Taster mit Details ausgeben
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (isPressed(id)) {
            uint8_t ic = (id - 1) / 8;
            uint8_t btnBit = 7 - ((id - 1) % 8);  // BTN: MSB=T1 (Bit7)
            uint8_t ledBit = (id - 1) % 8;        // LED: LSB=L1 (Bit0)
            
            Serial.print("T");
            if (id < 10) Serial.print(" ");
            Serial.print(id);
            Serial.print("->L");
            if (id < 10) Serial.print(" ");
            Serial.print(id);
            Serial.print(" | IC#");
            Serial.print(ic);
            Serial.print(" | BTN:Bit");
            Serial.print(btnBit);
            Serial.print("(MSB) LED:Bit");
            Serial.print(ledBit);
            Serial.print("(LSB) | 0x");
            if (btnRaw[ic] < 0x10) Serial.print("0");
            Serial.print(btnRaw[ic], HEX);
            Serial.print("->0x");
            if (ledState[ic] < 0x10) Serial.print("0");
            Serial.print(ledState[ic], HEX);
            Serial.print(" | BTN:");
            printBinary(btnRaw, BTN_BYTES);
            Serial.print(" LED:");
            printBinary(ledState, LED_BYTES);
            Serial.println();
        }
    }
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 4: Kombiniert (Taster -> LED)");
    Serial.println("========================================");
    Serial.println();
    Serial.print("Konfiguration: ");
    Serial.print(BTN_COUNT);
    Serial.print(" Taster, ");
    Serial.print(LED_COUNT);
    Serial.println(" LEDs");
    Serial.print("Hardware: ");
    Serial.print(BTN_ICS);
    Serial.print("x CD4021B, ");
    Serial.print(LED_ICS);
    Serial.println("x 74HC595");
    Serial.print("PWM Helligkeit: ");
    Serial.print(PWM_DUTY);
    Serial.println("%");
    Serial.println();
    
    // Pins initialisieren
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_BTN_PS, OUTPUT);
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
    pinMode(PIN_LED_MOSI, OUTPUT);
    pinMode(PIN_LED_RCK, OUTPUT);
    
    digitalWrite(PIN_SCK, LOW);
    digitalWrite(PIN_BTN_PS, LOW);
    digitalWrite(PIN_LED_RCK, LOW);
    
    // PWM Setup
    ledcSetup(LEDC_CHANNEL, PWM_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
    setLEDBrightness(PWM_DUTY);
    
    // Zustand initialisieren
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnRaw[i] = 0xFF;
        btnLast[i] = 0xFF;
    }
    clearLEDs();
    writeLEDs();
    
    Serial.println("Bereit. Druecke Taster...");
    Serial.println();
}

// =============================================================================
// Loop
// =============================================================================

void loop() {
    // Taster lesen
    readButtons();
    
    // Taster → LED zuordnen
    for (uint8_t id = 1; id <= LED_COUNT; ++id) {
        setLED(id, isPressed(id));
    }
    
    // LEDs aktualisieren
    writeLEDs();
    
    // Debug-Ausgabe nur bei Änderung und wenn Taster gedrückt
    if (btnChanged() && anyPressed()) {
        printState();
    }
    btnSave();
    
    delay(LOOP_DELAY_MS);
}
