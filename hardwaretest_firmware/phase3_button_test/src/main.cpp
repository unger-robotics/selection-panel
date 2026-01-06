/**
 * @file main.cpp
 * @brief Taster-Test über Schieberegister CD4021B (Active-Low)
 * @version 2.2.0
 * 
 * Hardware: 2x CD4021B in Daisy-Chain (10 Taster)
 * Active-Low: Taster gedrückt = Bit ist 0
 * 
 * Bit-Zuordnung (Hardware-bedingt):
 * Bit 7 = Taster 1, Bit 6 = Taster 2, ... Bit 0 = Taster 8
 * (gespiegelt zu LED, da CD4021B anders verdrahtet als 74HC595)
 * 
 * Hex-Referenz (Active-Low, Bit7 = Taster 1):
 * ┌────────┬──────┬────────────┐
 * │ Taster │ Hex  │ Binär      │
 * ├────────┼──────┼────────────┤
 * │   1    │ 0x7F │ 0111 1111  │
 * │   2    │ 0xBF │ 1011 1111  │
 * │   3    │ 0xDF │ 1101 1111  │
 * │   4    │ 0xEF │ 1110 1111  │
 * │   5    │ 0xF7 │ 1111 0111  │
 * │   6    │ 0xFB │ 1111 1011  │
 * │   7    │ 0xFD │ 1111 1101  │
 * │   8    │ 0xFE │ 1111 1110  │
 * │  9-10  │ IC1  │ wie IC0    │
 * │  kein  │ 0xFF │ 1111 1111  │
 * └────────┴──────┴────────────┘
 */

#include <Arduino.h>

// =============================================================================
// Konfiguration
// =============================================================================

constexpr size_t BTN_COUNT = 10;

// Pins (XIAO ESP32-S3)
// Warum diese Pins? Shared SPI-Bus mit 74HC595 für LEDs
constexpr int PIN_SCK = D8;       // Clock (shared)
constexpr int PIN_BTN_PS = D1;    // Parallel/Serial
constexpr int PIN_BTN_MISO = D9;  // Daten vom IC

// LED-Pins (nur zum Ausschalten)
constexpr int PIN_LED_MOSI = D10;
constexpr int PIN_LED_RCK = D0;
constexpr int PIN_LED_OE = D2;

// =============================================================================
// Zustand
// =============================================================================

constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8;
static uint8_t btnRaw[BTN_BYTES];
static uint8_t btnLast[BTN_BYTES];

// =============================================================================
// CD4021B Lesen
// =============================================================================

static inline void clockPulse() {
    // Warum 2µs? CD4021B braucht min. 180ns, 2µs ist sicher
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(2);
}

static void readButtons() {
    // Parallel Load: Alle 8 Eingänge gleichzeitig übernehmen
    // Warum HIGH→LOW? Fallende Flanke triggert den Latch
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(2);
    
    // Serial Shift
    // Warum erst lesen, dann clocken? Q8 liegt nach Load SOFORT am Ausgang
    // (Hardware-SPI würde erstes Bit verlieren)
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
    // 0xFF = kein Taster gedrückt (Active-Low)
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnRaw[i] != 0xFF) return true;
    }
    return false;
}

// =============================================================================
// Ausgabe
// =============================================================================

static void printPressed() {
    // IC0
    Serial.print("IC0: ");
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((btnRaw[0] >> bit) & 1);
    }
    Serial.print(" (0x");
    if (btnRaw[0] < 0x10) Serial.print("0");
    Serial.print(btnRaw[0], HEX);
    Serial.print(")");
    
    // IC1
    Serial.print(" | IC1: ");
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((btnRaw[1] >> bit) & 1);
    }
    Serial.print(" (0x");
    if (btnRaw[1] < 0x10) Serial.print("0");
    Serial.print(btnRaw[1], HEX);
    Serial.print(")");
    
    // Taster-Liste
    // Bit7 = T1, Bit6 = T2, ... (Hardware-bedingt, CD4021B Verdrahtung)
    Serial.print(" | Taster: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        uint8_t ic = (id - 1) / 8;
        uint8_t bit = 7 - ((id - 1) % 8);  // Bit7=T1, Bit6=T2, ...
        // Warum invertiert? Active-Low: Bit=0 bedeutet gedrückt
        bool pressed = !(btnRaw[ic] & (1 << bit));
        if (pressed) {
            if (any) Serial.print(",");
            Serial.print(id);
            any = true;
        }
    }
    if (!any) Serial.print("-");
    Serial.println();
}

// =============================================================================
// LEDs ausschalten
// =============================================================================

static void disableLEDs() {
    // Warum nötig? 74HC595 behält Latch-Zustand nach Reset
    pinMode(PIN_LED_MOSI, OUTPUT);
    pinMode(PIN_LED_RCK, OUTPUT);
    pinMode(PIN_LED_OE, OUTPUT);
    
    digitalWrite(PIN_LED_OE, HIGH);  // Ausgänge deaktivieren
    digitalWrite(PIN_LED_MOSI, LOW);
    
    // 16 Nullen einschieben (2x 74HC595)
    for (int i = 0; i < 16; ++i) {
        clockPulse();
    }
    
    // Latch: Nullen übernehmen
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_LED_RCK, LOW);
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Taster-Test - CD4021B (Active-Low)");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Hex-Referenz (Bit7=T1, Hardware-bedingt):");
    Serial.println("T1=0x7F T2=0xBF T3=0xDF T4=0xEF");
    Serial.println("T5=0xF7 T6=0xFB T7=0xFD T8=0xFE");
    Serial.println("T9/T10 auf IC1 (gleiche Werte)");
    Serial.println();
    Serial.println("Warte auf Tastendruck...");
    Serial.println();
    
    // Taster-Pins
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_BTN_PS, OUTPUT);
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
    digitalWrite(PIN_SCK, LOW);
    digitalWrite(PIN_BTN_PS, LOW);
    
    // LEDs aus
    disableLEDs();
    
    // Init: Alle Taster "nicht gedrückt" (0xFF = alle Bits HIGH)
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnRaw[i] = 0xFF;
        btnLast[i] = 0xFF;
    }
}

// =============================================================================
// Loop
// =============================================================================

void loop() {
    readButtons();
    
    // Nur ausgeben wenn: Änderung UND mindestens ein Taster gedrückt
    if (btnChanged() && anyPressed()) {
        printPressed();
    }
    btnSave();
    
    delay(10);
}
