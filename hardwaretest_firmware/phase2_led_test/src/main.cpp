/**
 * =============================================================================
 * Phase 2: LED-Ausgabe über 74HC595 Schieberegister
 * =============================================================================
 *
 * Ziel: Verifizieren, dass die LED-Ausgabe über 74HC595 funktioniert
 *
 * Hardware-Aufbau:
 *   ESP32-S3 XIAO          74HC595 (#0)         74HC595 (#1)
 *   ─────────────          ────────────         ────────────
 *   D0 (GPIO1) ──────────► SER (Pin 14)
 *   D1 (GPIO2) ──────────► SRCLK (Pin 11) ◄───► SRCLK (Pin 11)
 *   D2 (GPIO3) ──────────► RCLK (Pin 12)  ◄───► RCLK (Pin 12)
 *                          QH' (Pin 9) ───────► SER (Pin 14)
 *   3V3 ─────────────────► VCC (Pin 16)   ◄───► VCC (Pin 16)
 *   3V3 ─────────────────► SRCLR (Pin 10) ◄───► SRCLR (Pin 10)
 *   GND ─────────────────► GND (Pin 8)    ◄───► GND (Pin 8)
 *   GND ─────────────────► OE (Pin 13)    ◄───► OE (Pin 13)
 *
 *   LEDs mit 330Ω Vorwiderstand an QA-QH (Pin 15, 1-7)
 *
 * Tests via Serial:
 *   LED n      -> Schaltet LED n ein (0-9), alle anderen aus
 *   ALL        -> Alle LEDs ein
 *   CLEAR      -> Alle LEDs aus
 *   CHASE      -> Lauflicht starten/stoppen
 *   BINARY n   -> Zeigt Zahl n binär (0-255)
 *   TEST       -> Durchläuft alle LEDs einzeln
 *
 * =============================================================================
 */

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin-Definitionen (ESP32-S3 XIAO)
// -----------------------------------------------------------------------------
constexpr uint8_t PIN_LED_DATA = D0;  // GPIO1 -> 74HC595 SER
constexpr uint8_t PIN_LED_CLK = D1;   // GPIO2 -> 74HC595 SRCLK
constexpr uint8_t PIN_LED_LATCH = D2; // GPIO3 -> 74HC595 RCLK

// -----------------------------------------------------------------------------
// Konfiguration
// -----------------------------------------------------------------------------
constexpr uint8_t NUM_SHIFT_REGS = 2; // 2x 74HC595 = 16 Bits
constexpr uint8_t NUM_LEDS = 10;      // 10 LEDs im Prototyp
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t CHASE_INTERVAL_MS = 150;

// -----------------------------------------------------------------------------
// Globale Variablen
// -----------------------------------------------------------------------------
uint16_t ledState = 0; // Aktueller LED-Zustand (16 Bit)
bool chaseMode = false;
uint8_t chaseIndex = 0;
uint32_t lastChaseTime = 0;

// Forward Declaration
void printStartup();

// -----------------------------------------------------------------------------
// Schieberegister-Funktionen
// -----------------------------------------------------------------------------

/**
 * Schiebt 16 Bit in die beiden kaskadierten 74HC595
 * MSB zuerst -> landet im zweiten IC
 */
void shiftOut16(uint16_t data) {
    // Latch LOW halten während des Schiebens
    digitalWrite(PIN_LED_LATCH, LOW);

    // 16 Bits schieben, MSB zuerst
    for (int8_t i = 15; i >= 0; i--) {
        // Datenbit setzen
        digitalWrite(PIN_LED_DATA, (data >> i) & 0x01);

        // Clock-Puls: LOW -> HIGH -> LOW
        digitalWrite(PIN_LED_CLK, LOW);
        delayMicroseconds(1);
        digitalWrite(PIN_LED_CLK, HIGH);
        delayMicroseconds(1);
    }

    // Latch HIGH -> Daten werden an Ausgänge übernommen
    digitalWrite(PIN_LED_LATCH, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_LED_LATCH, LOW);
}

/**
 * Aktualisiert die Hardware mit dem aktuellen ledState
 */
void updateLEDs() { shiftOut16(ledState); }

/**
 * Setzt eine einzelne LED (One-Hot: nur diese LED an)
 */
void setLED(uint8_t index) {
    if (index < NUM_LEDS) {
        ledState = (1 << index);
        updateLEDs();
        Serial.printf("LED %d ON (Pattern: 0x%04X)\n", index, ledState);
    } else {
        Serial.printf("ERROR: LED %d out of range (0-%d)\n", index,
                      NUM_LEDS - 1);
    }
}

/**
 * Alle LEDs ein
 */
void setAllLEDs() {
    ledState = (1 << NUM_LEDS) - 1; // z.B. 0x03FF für 10 LEDs
    updateLEDs();
    Serial.printf("ALL LEDs ON (Pattern: 0x%04X)\n", ledState);
}

/**
 * Alle LEDs aus
 */
void clearLEDs() {
    ledState = 0;
    updateLEDs();
    Serial.println("ALL LEDs OFF");
}

/**
 * Zeigt eine Zahl binär an
 */
void showBinary(uint16_t value) {
    ledState = value & ((1 << NUM_LEDS) - 1);
    updateLEDs();
    Serial.printf("Binary: %d = 0x%04X = ", value, ledState);
    for (int8_t i = NUM_LEDS - 1; i >= 0; i--) {
        Serial.print((ledState >> i) & 1);
    }
    Serial.println();
}

/**
 * Durchläuft alle LEDs einzeln
 */
void runTest() {
    Serial.println("Running LED test...");
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        ledState = (1 << i);
        updateLEDs();
        Serial.printf("  LED %d\n", i);
        delay(300);
    }
    clearLEDs();
    Serial.println("Test complete.");
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

        if (input.startsWith("LED ")) {
            int index = input.substring(4).toInt();
            chaseMode = false;
            setLED(index);
        } else if (input == "ALL") {
            chaseMode = false;
            setAllLEDs();
        } else if (input == "CLEAR" || input == "OFF") {
            chaseMode = false;
            clearLEDs();
        } else if (input == "CHASE") {
            chaseMode = !chaseMode;
            Serial.printf("Chase mode: %s\n", chaseMode ? "ON" : "OFF");
            if (!chaseMode)
                clearLEDs();
        } else if (input.startsWith("BINARY ")) {
            int value = input.substring(7).toInt();
            chaseMode = false;
            showBinary(value);
        } else if (input == "TEST") {
            chaseMode = false;
            runTest();
        } else if (input == "HELP") {
            Serial.println();
            Serial.println("Befehle:");
            Serial.println("  LED n    - LED n einschalten (0-9)");
            Serial.println("  ALL      - Alle LEDs ein");
            Serial.println("  CLEAR    - Alle LEDs aus");
            Serial.println("  CHASE    - Lauflicht ein/aus");
            Serial.println("  BINARY n - Zahl binär anzeigen");
            Serial.println("  TEST     - Alle LEDs durchlaufen");
            Serial.println("  HELLO    - Startup-Nachricht anzeigen");
            Serial.println();
        } else if (input == "HELLO") {
            printStartup();
        } else if (input == "PING") {
            Serial.println("PONG");
        } else {
            Serial.printf("Unknown command: %s (try HELP)\n", input.c_str());
        }
    }
}

/**
 * Lauflicht-Logik
 */
void updateChase() {
    if (!chaseMode)
        return;

    uint32_t now = millis();
    if (now - lastChaseTime >= CHASE_INTERVAL_MS) {
        lastChaseTime = now;

        ledState = (1 << chaseIndex);
        updateLEDs();

        chaseIndex++;
        if (chaseIndex >= NUM_LEDS) {
            chaseIndex = 0;
        }
    }
}

// -----------------------------------------------------------------------------
// Startup-Nachricht
// -----------------------------------------------------------------------------
void printStartup() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 2: LED-Test (74HC595)");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Pinbelegung:");
    Serial.printf("  LED Data  (D0/GPIO%d) -> 74HC595 SER\n", PIN_LED_DATA);
    Serial.printf("  LED Clock (D1/GPIO%d) -> 74HC595 SRCLK\n", PIN_LED_CLK);
    Serial.printf("  LED Latch (D2/GPIO%d) -> 74HC595 RCLK\n", PIN_LED_LATCH);
    Serial.println();
    Serial.printf("Konfiguration: %d Schieberegister, %d LEDs\n",
                  NUM_SHIFT_REGS, NUM_LEDS);
    Serial.println();
    Serial.println(
        "Befehle: LED n, ALL, CLEAR, CHASE, BINARY n, TEST, HELLO, HELP");
    Serial.println();
    Serial.println("READY");
    Serial.println();
}

// -----------------------------------------------------------------------------
// Setup & Loop
// -----------------------------------------------------------------------------
void setup() {
    // Serial initialisieren
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    // Pins konfigurieren
    pinMode(PIN_LED_DATA, OUTPUT);
    pinMode(PIN_LED_CLK, OUTPUT);
    pinMode(PIN_LED_LATCH, OUTPUT);

    // Initiale Zustände
    digitalWrite(PIN_LED_DATA, LOW);
    digitalWrite(PIN_LED_CLK, LOW);
    digitalWrite(PIN_LED_LATCH, LOW);

    // LEDs ausschalten
    clearLEDs();

    // Startup-Nachricht
    printStartup();

    // Kurzer Test: Alle LEDs kurz aufleuchten lassen
    setAllLEDs();
    delay(500);
    clearLEDs();
}

void loop() {
    handleSerialInput();
    updateChase();

    // WICHTIG: Watchdog füttern - ESP32-S3 USB-CDC braucht das!
    delay(1);
}
