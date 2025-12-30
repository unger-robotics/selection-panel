/**
 * =============================================================================
 * Phase 3: Taster-Eingabe über CD4021BE Schieberegister
 * =============================================================================
 *
 * Ziel: Verifizieren, dass die Taster-Eingabe über CD4021BE funktioniert
 *
 * Hardware-Aufbau:
 *   ESP32-S3 XIAO          CD4021BE (#0)        CD4021BE (#1)
 *   ─────────────          ─────────────        ─────────────
 *   D3 (GPIO4) ◄────────── Q8 (Pin 3)
 *   D4 (GPIO5) ──────────► CLK (Pin 10)   ◄───► CLK (Pin 10)
 *   D5 (GPIO6) ──────────► P/S (Pin 9)    ◄───► P/S (Pin 9)
 *                          SER (Pin 11) ◄────── Q8 (Pin 3)
 *   3V3 ─────────────────► VDD (Pin 16)   ◄───► VDD (Pin 16)
 *   3V3 ────────────────────────────────────────► SER (Pin 11) am LETZTEN IC!
 *   GND ─────────────────► VSS (Pin 8)    ◄───► VSS (Pin 8)
 *
 *   Taster zwischen P1-P8 (Pins 1,13,14,4,5,6,7,15) und GND
 *   Pull-Up 10kΩ von jedem Px nach VDD
 *
 * WICHTIG: CD4021BE Load-Logik ist INVERTIERT zum 74HC165!
 *   P/S = HIGH -> Parallel Load (Taster einlesen)
 *   P/S = LOW  -> Serial Shift (Daten auslesen)
 *
 * Tests via Serial:
 *   SCAN       -> Einmaliges Auslesen aller Taster
 *   MONITOR    -> Kontinuierliches Monitoring (Toggle)
 *   RAW        -> Zeigt Roh-Bits
 *   HELP       -> Hilfe
 *
 * =============================================================================
 */

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin-Definitionen (ESP32-S3 XIAO)
// -----------------------------------------------------------------------------
// LED-Ausgabe (74HC595) - nur zum Ausschalten
constexpr uint8_t PIN_LED_DATA = D0;  // GPIO1 -> 74HC595 SER
constexpr uint8_t PIN_LED_CLK = D1;   // GPIO2 -> 74HC595 SRCLK
constexpr uint8_t PIN_LED_LATCH = D2; // GPIO3 -> 74HC595 RCLK

// Taster-Eingabe (CD4021BE)
constexpr uint8_t PIN_BTN_DATA = D3; // GPIO4 <- CD4021 Q8
constexpr uint8_t PIN_BTN_CLK = D4;  // GPIO5 -> CD4021 CLK
constexpr uint8_t PIN_BTN_LOAD = D5; // GPIO6 -> CD4021 P/S

// -----------------------------------------------------------------------------
// Konfiguration
// -----------------------------------------------------------------------------
constexpr uint8_t NUM_SHIFT_REGS = 2; // 2x CD4021BE = 16 Bits
constexpr uint8_t NUM_BUTTONS = 10;   // 10 Taster im Prototyp
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SCAN_INTERVAL_MS = 50;
constexpr uint32_t DEBOUNCE_MS = 50;

// -----------------------------------------------------------------------------
// Bit-Mapping: Physischer Taster -> Bit-Position im Schieberegister
// -----------------------------------------------------------------------------
constexpr uint8_t BUTTON_BIT_MAP[10] = {
    15, // Taster 1 -> Bit 15 (IC#1)
    12, // Taster 2 -> Bit 12 (IC#1)
    13, // Taster 3 -> Bit 13 (IC#1)
    11, // Taster 4 -> Bit 11 (IC#1)
    10, // Taster 5 -> Bit 10 (IC#1)
    9,  // Taster 6 -> Bit 9  (IC#1)
    8,  // Taster 7 -> Bit 8  (IC#1)
    14, // Taster 8 -> Bit 14 (IC#1)
    7,  // Taster 9 -> Bit 7  (IC#0)
    4   // Taster 10-> Bit 4  (IC#0)
};

// -----------------------------------------------------------------------------
// Globale Variablen
// -----------------------------------------------------------------------------
uint16_t buttonState = 0;            // Aktueller Taster-Zustand
uint16_t lastRawState = 0xFFFF;      // Letzter Roh-Zustand
uint32_t lastDebounceTime[10] = {0}; // Debounce-Timer pro Taste
bool monitorMode = false;
uint32_t lastScanTime = 0;

// Forward Declaration
void printStartup();

// -----------------------------------------------------------------------------
// Schieberegister-Funktionen
// -----------------------------------------------------------------------------

/**
 * Liest 16 Bit aus den kaskadierten CD4021BE
 */
uint16_t shiftIn16() {
    uint16_t data = 0;

    // Parallel Load: P/S = HIGH
    digitalWrite(PIN_BTN_LOAD, HIGH);
    delayMicroseconds(5);

    // Serial Mode: P/S = LOW
    digitalWrite(PIN_BTN_LOAD, LOW);
    delayMicroseconds(2);

    // 16 Bits einlesen
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t bit = digitalRead(PIN_BTN_DATA);
        data = (data << 1) | bit;

        digitalWrite(PIN_BTN_CLK, HIGH);
        delayMicroseconds(2);
        digitalWrite(PIN_BTN_CLK, LOW);
        delayMicroseconds(2);
    }

    return data;
}

/**
 * Prüft Taster mit Bit-Mapping und Debouncing
 */
void checkButtonChanges() {
    uint16_t raw = shiftIn16();
    uint32_t now = millis();

    for (uint8_t taster = 0; taster < NUM_BUTTONS; taster++) {
        uint8_t bitPos = BUTTON_BIT_MAP[taster];

        // Bit auslesen und invertieren (gedrückt = LOW = 0, wir wollen 1)
        bool currentPressed = !((raw >> bitPos) & 1);
        bool lastPressed = (buttonState >> taster) & 1;

        if (currentPressed != lastPressed) {
            if (now - lastDebounceTime[taster] > DEBOUNCE_MS) {
                lastDebounceTime[taster] = now;

                if (currentPressed) {
                    buttonState |= (1 << taster);
                    Serial.printf("PRESS %d\n", taster + 1); // 1-basiert
                } else {
                    buttonState &= ~(1 << taster);
                    Serial.printf("RELEASE %d\n", taster + 1);
                }
            }
        } else {
            lastDebounceTime[taster] = now;
        }
    }

    lastRawState = raw;
}

/**
 * Einmaliges Auslesen und Anzeigen
 */
void scanButtons() {
    uint16_t raw = shiftIn16();

    Serial.println();
    Serial.println("Button Scan:");
    Serial.printf("  Raw data: 0x%04X\n", raw);
    Serial.println();

    // Gemappte Taster anzeigen
    Serial.println("  Taster-Status (mit Mapping):");
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        uint8_t bitPos = BUTTON_BIT_MAP[i];
        bool pressed = !((raw >> bitPos) & 1);
        Serial.printf("    Taster %2d -> Bit %2d: %s\n", i + 1, bitPos,
                      pressed ? "GEDRUECKT" : "-");
    }
    Serial.println();

    // Gedrückte Taster
    Serial.print("  Gedrueckt: ");
    bool anyPressed = false;
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        uint8_t bitPos = BUTTON_BIT_MAP[i];
        if (!((raw >> bitPos) & 1)) {
            if (anyPressed)
                Serial.print(", ");
            Serial.printf("%d", i + 1);
            anyPressed = true;
        }
    }
    if (!anyPressed)
        Serial.print("keine");
    Serial.println();
    Serial.println();
}

/**
 * Zeigt Roh-Bits für Debugging
 */
void showRawBits() {
    uint16_t raw = shiftIn16();

    Serial.println();
    Serial.println("Raw Bits (MSB first):");
    Serial.print("  IC#1[");
    for (int8_t i = 15; i >= 8; i--) {
        Serial.print((raw >> i) & 1);
    }
    Serial.print("] IC#0[");
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((raw >> i) & 1);
    }
    Serial.println("]");
    Serial.printf("  Hex: 0x%04X\n", raw);
    Serial.println();
}

// -----------------------------------------------------------------------------
// Serial-Kommandos
// -----------------------------------------------------------------------------
void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        if (input.length() == 0)
            return;

        if (input == "SCAN" || input == "S") {
            scanButtons();
        } else if (input == "MONITOR" || input == "M") {
            monitorMode = !monitorMode;
            Serial.printf("Monitor mode: %s\n", monitorMode ? "ON" : "OFF");
            if (monitorMode) {
                Serial.println("(Taster druecken, MONITOR zum Stoppen)");
            }
        } else if (input == "RAW" || input == "R") {
            showRawBits();
        } else if (input == "HELP" || input == "H") {
            Serial.println();
            Serial.println("Befehle:");
            Serial.println("  SCAN (S)     - Einmaliges Auslesen");
            Serial.println("  MONITOR (M)  - Kontinuierliches Monitoring");
            Serial.println("  RAW (R)      - Roh-Bits anzeigen");
            Serial.println("  HELLO        - Startup-Nachricht anzeigen");
            Serial.println("  HELP (H)     - Diese Hilfe");
            Serial.println();
        } else if (input == "HELLO") {
            printStartup();
        } else if (input == "PING") {
            Serial.println("PONG");
        } else {
            Serial.printf("Unknown: %s (try HELP)\n", input.c_str());
        }
    }
}

// -----------------------------------------------------------------------------
// Setup & Loop
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    // LED-Pins (um LEDs auszuschalten)
    pinMode(PIN_LED_DATA, OUTPUT);
    pinMode(PIN_LED_CLK, OUTPUT);
    pinMode(PIN_LED_LATCH, OUTPUT);
    digitalWrite(PIN_LED_DATA, LOW);
    digitalWrite(PIN_LED_CLK, LOW);
    digitalWrite(PIN_LED_LATCH, LOW);

    // LEDs ausschalten
    for (uint8_t i = 0; i < 16; i++) {
        digitalWrite(PIN_LED_CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(PIN_LED_CLK, LOW);
    }
    digitalWrite(PIN_LED_LATCH, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_LED_LATCH, LOW);

    // Button-Pins
    pinMode(PIN_BTN_DATA, INPUT);
    pinMode(PIN_BTN_CLK, OUTPUT);
    pinMode(PIN_BTN_LOAD, OUTPUT);
    digitalWrite(PIN_BTN_CLK, LOW);
    digitalWrite(PIN_BTN_LOAD, LOW);

    printStartup();
    scanButtons();
}

void printStartup() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 3: Taster-Test (CD4021BE)");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Pinbelegung:");
    Serial.printf("  Btn Data (D3/GPIO%d) <- CD4021 Q8\n", PIN_BTN_DATA);
    Serial.printf("  Btn Clock(D4/GPIO%d) -> CD4021 CLK\n", PIN_BTN_CLK);
    Serial.printf("  Btn Load (D5/GPIO%d) -> CD4021 P/S\n", PIN_BTN_LOAD);
    Serial.println();
    Serial.println("Bit-Mapping:");
    Serial.println("  Taster 1-8  -> Bits 15,12,13,11,10,9,8,14 (IC#1)");
    Serial.println("  Taster 9-10 -> Bits 7,4 (IC#0)");
    Serial.println();
    Serial.println("WICHTIG: Unbenutzte Eingaenge -> VCC!");
    Serial.println();
    Serial.println("Befehle: SCAN, MONITOR, RAW, HELLO, HELP");
    Serial.println();
    Serial.println("READY");
    Serial.println();
}

void loop() {
    handleSerialInput();

    if (monitorMode) {
        uint32_t now = millis();
        if (now - lastScanTime >= SCAN_INTERVAL_MS) {
            lastScanTime = now;
            checkButtonChanges();
        }
    }

    delay(1);
}
