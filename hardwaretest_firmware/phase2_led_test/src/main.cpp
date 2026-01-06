/**
 * @file main.cpp
 * @brief LED-Test über Schieberegister 74HC595 (Active-High)
 * @version 2.0.0
 * 
 * Hardware: 2x 74HC595 in Daisy-Chain (10 LEDs)
 * Active-High: LED an = Bit ist 1
 * 
 * Hex-Referenz (Active-High, Bit0 = LED 1):
 * ┌──────┬──────┬────────────┐
 * │ LED  │ Hex  │ Binär      │
 * ├──────┼──────┼────────────┤
 * │  1   │ 0x01 │ 0000 0001  │
 * │  2   │ 0x02 │ 0000 0010  │
 * │  3   │ 0x04 │ 0000 0100  │
 * │  4   │ 0x08 │ 0000 1000  │
 * │  5   │ 0x10 │ 0001 0000  │
 * │  6   │ 0x20 │ 0010 0000  │
 * │  7   │ 0x40 │ 0100 0000  │
 * │  8   │ 0x80 │ 1000 0000  │
 * │ 9-10 │ IC1  │ wie IC0    │
 * │ alle │ 0xFF │ 1111 1111  │
 * │ keine│ 0x00 │ 0000 0000  │
 * └──────┴──────┴────────────┘
 */

#include <Arduino.h>

// =============================================================================
// Konfiguration
// =============================================================================

constexpr size_t LED_COUNT = 10;
constexpr uint8_t PWM_DUTY = 50;         // Helligkeit (%)
constexpr uint32_t PWM_FREQ = 1000;      // PWM-Frequenz (Hz)
constexpr uint32_t CHASE_DELAY_MS = 300; // Lauflicht-Geschwindigkeit

// Pins (XIAO ESP32-S3)
// Warum diese Pins? Shared SPI-Bus mit CD4021B für Taster
constexpr int PIN_SCK = D8;       // Clock (shared)
constexpr int PIN_LED_MOSI = D10; // Daten zum IC
constexpr int PIN_LED_RCK = D0;   // Latch
constexpr int PIN_LED_OE = D2;    // PWM Helligkeit

// Taster-Pins (zum Deaktivieren)
constexpr int PIN_BTN_PS = D1;
constexpr int PIN_BTN_MISO = D9;

// LEDC PWM
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint8_t LEDC_RESOLUTION = 8;

// =============================================================================
// Zustand
// =============================================================================

constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8;
static uint8_t ledState[LED_BYTES];
static uint8_t currentLED = 1;

// =============================================================================
// 74HC595 Schreiben
// =============================================================================

static inline void clockPulse() {
    // Warum 2µs? 74HC595 braucht min. 20ns, 2µs ist sicher
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(2);
}

static void writeLEDs() {
    // Warum rückwärts? Daisy-Chain: Letztes IC zuerst schieben
    for (int16_t ic = LED_BYTES - 1; ic >= 0; --ic) {
        uint8_t byte = ledState[ic];
        // Warum Bit7 zuerst? MSB-first entspricht 74HC595 Pinout
        for (int8_t bit = 7; bit >= 0; --bit) {
            digitalWrite(PIN_LED_MOSI, (byte >> bit) & 1);
            clockPulse();
        }
    }
    
    // Latch: Daten in Ausgaberegister übernehmen
    // Warum nötig? 74HC595 hat Shift- und Storage-Register getrennt
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_LED_RCK, LOW);
}

// =============================================================================
// PWM
// =============================================================================

static void setLEDBrightness(uint8_t percent) {
    // Warum invertiert? OE ist Active-LOW (LOW = Ausgänge aktiv)
    // duty 0 = voll an, duty 255 = aus
    uint8_t duty = 255 - (percent * 255 / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

// =============================================================================
// LED-API
// =============================================================================

static void setLED(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT) return;
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = (id - 1) % 8;
    // Warum so? Bit0 = LED1, Bit1 = LED2, ... (Active-High)
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
// Ausgabe
// =============================================================================

static void printLEDState() {
    // IC0
    Serial.print("IC0: ");
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((ledState[0] >> bit) & 1);
    }
    Serial.print(" (0x");
    if (ledState[0] < 0x10) Serial.print("0");
    Serial.print(ledState[0], HEX);
    Serial.print(")");
    
    // IC1
    Serial.print(" | IC1: ");
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((ledState[1] >> bit) & 1);
    }
    Serial.print(" (0x");
    if (ledState[1] < 0x10) Serial.print("0");
    Serial.print(ledState[1], HEX);
    Serial.print(")");
    
    // LED-Liste
    Serial.print(" | LED: ");
    Serial.println(currentLED);
}

// =============================================================================
// Taster deaktivieren
// =============================================================================

static void disableButtons() {
    // Warum nötig? CD4021B P/S-Pin auf definiertem Zustand halten
    // P/S LOW = Shift-Modus (kein versehentliches Laden)
    pinMode(PIN_BTN_PS, OUTPUT);
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
    digitalWrite(PIN_BTN_PS, LOW);
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("LED-Test - 74HC595 (Active-High)");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Hex-Referenz:");
    Serial.println("L1=0x01 L2=0x02 L3=0x04 L4=0x08");
    Serial.println("L5=0x10 L6=0x20 L7=0x40 L8=0x80");
    Serial.println("L9/L10 auf IC1 (gleiche Werte)");
    Serial.println();
    Serial.print("PWM Helligkeit: ");
    Serial.print(PWM_DUTY);
    Serial.println("%");
    Serial.println();
    
    // LED-Pins
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_LED_MOSI, OUTPUT);
    pinMode(PIN_LED_RCK, OUTPUT);
    digitalWrite(PIN_SCK, LOW);
    digitalWrite(PIN_LED_RCK, LOW);
    
    // PWM Setup
    ledcSetup(LEDC_CHANNEL, PWM_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
    setLEDBrightness(PWM_DUTY);
    
    // Taster deaktivieren
    disableButtons();
    
    // LEDs initialisieren
    clearLEDs();
    writeLEDs();
    
    Serial.println("Starte Lauflicht...");
    Serial.println();
}

// =============================================================================
// Loop
// =============================================================================

void loop() {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    
    if (now - lastUpdate >= CHASE_DELAY_MS) {
        lastUpdate = now;
        
        // Lauflicht: Eine LED nach der anderen
        clearLEDs();
        setLED(currentLED, true);
        writeLEDs();
        
        // Ausgabe
        printLEDState();
        
        // Nächste LED
        currentLED++;
        if (currentLED > LED_COUNT) {
            currentLED = 1;
        }
    }
}
