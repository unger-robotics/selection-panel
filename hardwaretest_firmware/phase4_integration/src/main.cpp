/**
 * =============================================================================
 * Phase 4: Integration - Taster → LED über Schieberegister
 * =============================================================================
 *
 * Ziel: Taster n schaltet LED n ein (One-Hot)
 *
 * Hardware-Aufbau:
 *   Alle Komponenten aus Phase 2 und Phase 3 kombiniert:
 *
 *   ESP32-S3 XIAO
 *   ─────────────
 *   D0 -> 74HC595 SER      (LED Data)
 *   D1 -> 74HC595 SRCLK    (LED Clock)
 *   D2 -> 74HC595 RCLK     (LED Latch)
 *   D3 <- CD4021  Q8       (Button Data)
 *   D4 -> CD4021  CLK      (Button Clock)
 *   D5 -> CD4021  P/S      (Button Load)
 *
 * Verhalten:
 *   - Taster drücken -> entsprechende LED leuchtet
 *   - One-Hot: Nur eine LED gleichzeitig (letzter Taster gewinnt)
 *   - Kein Taster gedrückt -> alle LEDs aus
 *
 * Tests via Serial:
 *   STATUS     -> Zeigt aktuellen Zustand
 *   LED n      -> Manuelle LED-Steuerung
 *   AUTO       -> Auto-Modus ein/aus (Taster → LED)
 *   HELP       -> Hilfe
 *
 * =============================================================================
 */

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Pin-Definitionen (ESP32-S3 XIAO)
// -----------------------------------------------------------------------------
// LED-Ausgabe (74HC595)
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
constexpr uint8_t NUM_CHANNELS = 10; // 10 Taster/LEDs im Prototyp
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t SCAN_INTERVAL_MS = 10;
constexpr uint32_t DEBOUNCE_MS = 50;

// -----------------------------------------------------------------------------
// Bit-Mapping: Physischer Taster -> Bit-Position im Schieberegister
// -----------------------------------------------------------------------------
// Taster 1-10 sind nicht linear verdrahtet, daher Mapping-Tabelle
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
uint16_t ledState = 0;
uint16_t buttonState = 0;
uint32_t lastDebounceTime[16] = {0};
bool autoMode = true;     // Taster steuert LED automatisch
int8_t activeButton = -1; // Aktuell gedrückter Taster (-1 = keiner)
uint32_t lastScanTime = 0;

// Forward Declaration
void printStartup();

// -----------------------------------------------------------------------------
// LED-Funktionen (74HC595)
// -----------------------------------------------------------------------------

void shiftOutLEDs(uint16_t data) {
    digitalWrite(PIN_LED_LATCH, LOW);

    for (int8_t i = 15; i >= 0; i--) {
        digitalWrite(PIN_LED_DATA, (data >> i) & 0x01);
        digitalWrite(PIN_LED_CLK, LOW);
        delayMicroseconds(1);
        digitalWrite(PIN_LED_CLK, HIGH);
        delayMicroseconds(1);
    }

    digitalWrite(PIN_LED_LATCH, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_LED_LATCH, LOW);
}

void setLED(int8_t index) {
    if (index < 0) {
        ledState = 0;
    } else if (index < NUM_CHANNELS) {
        ledState = (1 << index);
    }
    shiftOutLEDs(ledState);
}

void clearLEDs() {
    ledState = 0;
    shiftOutLEDs(ledState);
}

// -----------------------------------------------------------------------------
// Taster-Funktionen (CD4021BE)
// -----------------------------------------------------------------------------

uint16_t shiftInButtons() {
    uint16_t data = 0;

    // Parallel Load
    digitalWrite(PIN_BTN_LOAD, HIGH);
    delayMicroseconds(5);
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
 * Liest Taster mit Debouncing und Bit-Mapping
 * Gibt die Nummer des gedrückten Tasters zurück (-1 wenn keiner)
 */
int8_t scanButtons() {
    uint16_t raw = shiftInButtons();
    uint32_t now = millis();

    int8_t pressed = -1;

    for (uint8_t taster = 0; taster < NUM_CHANNELS; taster++) {
        // Bit-Position aus Mapping-Tabelle holen
        uint8_t bitPos = BUTTON_BIT_MAP[taster];

        // Bit auslesen und invertieren (gedrückt = LOW = 0, wir wollen 1)
        bool currentBit = !((raw >> bitPos) & 1);
        bool lastBit = (buttonState >> taster) & 1;

        if (currentBit != lastBit) {
            // Debounce-Check
            if (now - lastDebounceTime[taster] > DEBOUNCE_MS) {
                lastDebounceTime[taster] = now;

                if (currentBit) {
                    // Taste wurde gedrückt
                    buttonState |= (1 << taster);
                    Serial.printf("PRESS %d\n",
                                  taster + 1); // 1-basiert für Benutzer
                    pressed = taster;
                } else {
                    // Taste wurde losgelassen
                    buttonState &= ~(1 << taster);
                    Serial.printf("RELEASE %d\n", taster + 1);
                }
            }
        } else {
            // Reset debounce timer wenn stabil
            lastDebounceTime[taster] = now;
        }

        // Höchste gedrückte Taste finden (für One-Hot)
        if ((buttonState >> taster) & 1) {
            pressed = taster;
        }
    }

    return pressed;
}

/**
 * Findet den aktuell gedrückten Taster (für One-Hot)
 * Bei mehreren: der mit der höchsten Nummer gewinnt
 */
int8_t getActiveButton() {
    for (int8_t i = NUM_CHANNELS - 1; i >= 0; i--) {
        if ((buttonState >> i) & 1) {
            return i;
        }
    }
    return -1;
}

// -----------------------------------------------------------------------------
// Status-Anzeige
// -----------------------------------------------------------------------------

void showStatus() {
    Serial.println();
    Serial.println("=== Status ===");
    Serial.printf("Auto-Mode:      %s\n", autoMode ? "ON" : "OFF");
    Serial.printf("Active Button:  %d\n",
                  activeButton >= 0 ? activeButton + 1 : 0);
    Serial.printf("LED State:      0x%04X\n", ledState);
    Serial.printf("Button State:   0x%04X\n", buttonState);
    Serial.println();

    // Visuelle Darstellung (1-basiert für Benutzer)
    Serial.print("Buttons:  ");
    for (int8_t i = NUM_CHANNELS - 1; i >= 0; i--) {
        Serial.print((buttonState >> i) & 1 ? "X" : ".");
    }
    Serial.println(" (10..1)");

    Serial.print("LEDs:     ");
    for (int8_t i = NUM_CHANNELS - 1; i >= 0; i--) {
        Serial.print((ledState >> i) & 1 ? "*" : ".");
    }
    Serial.println(" (10..1)");
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

        if (input == "STATUS" || input == "S") {
            showStatus();
        } else if (input.startsWith("LED ")) {
            int index = input.substring(4).toInt();
            // Benutzer gibt 1-10 ein, intern 0-9
            if (index >= 1 && index <= NUM_CHANNELS) {
                setLED(index - 1); // Konvertiere zu 0-basiert
                Serial.printf("LED %d ON\n", index);
            } else {
                Serial.printf("ERROR: LED %d ungueltig (1-%d)\n", index,
                              NUM_CHANNELS);
            }
        } else if (input == "CLEAR" || input == "C") {
            clearLEDs();
            Serial.println("LEDs cleared");
        } else if (input == "AUTO" || input == "A") {
            autoMode = !autoMode;
            Serial.printf("Auto-mode: %s\n", autoMode ? "ON" : "OFF");
            if (!autoMode) {
                clearLEDs();
            }
        } else if (input == "HELP" || input == "H") {
            Serial.println();
            Serial.println("Befehle:");
            Serial.println("  STATUS (S)   - Zeigt aktuellen Zustand");
            Serial.println("  LED n        - Schaltet LED n ein (1-10)");
            Serial.println("  CLEAR (C)    - Alle LEDs aus");
            Serial.println("  AUTO (A)     - Auto-Modus ein/aus");
            Serial.println("  HELLO        - Startup-Nachricht anzeigen");
            Serial.println("  HELP (H)     - Diese Hilfe");
            Serial.println();
            Serial.println("Im Auto-Modus: Taster n -> LED n (1-10)");
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
// Startup-Nachricht
// -----------------------------------------------------------------------------

void printStartup() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 4: Integration (Taster -> LED)");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Pinbelegung:");
    Serial.println("  LED:    D0=Data, D1=Clock, D2=Latch");
    Serial.println("  Button: D3=Data, D4=Clock, D5=Load");
    Serial.println();
    Serial.printf("Konfiguration: %d Kanaele (1-%d)\n", NUM_CHANNELS,
                  NUM_CHANNELS);
    Serial.println();
    Serial.println("Verhalten:");
    Serial.println("  - Taster n druecken -> LED n leuchtet");
    Serial.println("  - One-Hot: Nur eine LED gleichzeitig");
    Serial.println("  - Kein Taster -> alle LEDs aus");
    Serial.println();
    Serial.println("Befehle: STATUS, LED n, CLEAR, AUTO, HELLO, HELP");
    Serial.println();
    Serial.println("READY");
    Serial.println();
}

// -----------------------------------------------------------------------------
// Setup & Loop
// -----------------------------------------------------------------------------

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    // LED-Pins (Output)
    pinMode(PIN_LED_DATA, OUTPUT);
    pinMode(PIN_LED_CLK, OUTPUT);
    pinMode(PIN_LED_LATCH, OUTPUT);
    digitalWrite(PIN_LED_DATA, LOW);
    digitalWrite(PIN_LED_CLK, LOW);
    digitalWrite(PIN_LED_LATCH, LOW);

    // Button-Pins
    pinMode(PIN_BTN_DATA, INPUT);
    pinMode(PIN_BTN_CLK, OUTPUT);
    pinMode(PIN_BTN_LOAD, OUTPUT);
    digitalWrite(PIN_BTN_CLK, LOW);
    digitalWrite(PIN_BTN_LOAD, LOW);

    // LEDs aus
    clearLEDs();

    // Startup
    printStartup();
}

void loop() {
    handleSerialInput();

    // Button-Scan
    uint32_t now = millis();
    if (now - lastScanTime >= SCAN_INTERVAL_MS) {
        lastScanTime = now;

        scanButtons();

        // Auto-Modus: Taster steuert LED
        if (autoMode) {
            int8_t newActive = getActiveButton();

            if (newActive != activeButton) {
                activeButton = newActive;
                setLED(activeButton);
            }
        }
    }

    // WICHTIG: Watchdog füttern - ESP32-S3 USB-CDC braucht das!
    delay(1);
}
