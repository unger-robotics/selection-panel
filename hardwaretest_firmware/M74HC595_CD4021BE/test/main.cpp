/**
 * =============================================================================
 * Diagnose: Taster-Bit-Mapping
 * =============================================================================
 *
 * Zeigt kontinuierlich alle 16 Bits und markiert Änderungen
 * =============================================================================
 */

#include <Arduino.h>

// Pin-Definitionen
constexpr uint8_t PIN_LED_DATA = D0;
constexpr uint8_t PIN_LED_CLK = D1;
constexpr uint8_t PIN_LED_LATCH = D2;
constexpr uint8_t PIN_BTN_DATA = D3;
constexpr uint8_t PIN_BTN_CLK = D4;
constexpr uint8_t PIN_BTN_LOAD = D5;

uint16_t lastRaw = 0xFFFF;

void clearLEDs() {
    pinMode(PIN_LED_DATA, OUTPUT);
    pinMode(PIN_LED_CLK, OUTPUT);
    pinMode(PIN_LED_LATCH, OUTPUT);
    digitalWrite(PIN_LED_DATA, LOW);
    for (uint8_t i = 0; i < 16; i++) {
        digitalWrite(PIN_LED_CLK, HIGH);
        delayMicroseconds(1);
        digitalWrite(PIN_LED_CLK, LOW);
    }
    digitalWrite(PIN_LED_LATCH, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_LED_LATCH, LOW);
}

uint16_t readRaw() {
    uint16_t data = 0;

    digitalWrite(PIN_BTN_LOAD, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_BTN_LOAD, LOW);
    delayMicroseconds(2);

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

void printBits(uint16_t data) {
    Serial.print("IC#1[");
    for (int8_t i = 15; i >= 8; i--) {
        Serial.print((data >> i) & 1);
    }
    Serial.print("] IC#0[");
    for (int8_t i = 7; i >= 0; i--) {
        Serial.print((data >> i) & 1);
    }
    Serial.print("]");
}

void printStartup() {
    Serial.println();
    Serial.println("=====================================");
    Serial.println("Phase 3D: TASTER DIAGNOSE");
    Serial.println("=====================================");
    Serial.println();
    Serial.println("Druecke jeden Taster einzeln!");
    Serial.println("Bit=0 bedeutet GEDRUECKT");
    Serial.println();
    Serial.println("Format: IC#1[Bit15-8] IC#0[Bit7-0]");
    Serial.println("        IC#1=kaskadiert, IC#0=am ESP32");
    Serial.println();
    Serial.println("Befehle: HELLO, HELP, PING");
    Serial.println();
    Serial.println("READY");
    Serial.println();

    Serial.print("Start: ");
    printBits(lastRaw);
    Serial.println();
    Serial.println();
}

void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();

        if (input.length() == 0)
            return;

        if (input == "HELLO") {
            printStartup();
        } else if (input == "HELP" || input == "H") {
            Serial.println();
            Serial.println("Befehle:");
            Serial.println("  HELLO  - Startup-Nachricht anzeigen");
            Serial.println("  HELP   - Diese Hilfe");
            Serial.println("  PING   - Antwortet mit PONG");
            Serial.println();
            Serial.println(
                "Taster druecken zeigt Bit-Aenderungen automatisch.");
            Serial.println();
        } else if (input == "PING") {
            Serial.println("PONG");
        } else {
            Serial.printf("Unknown command: %s (try HELP)\n", input.c_str());
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    clearLEDs();

    pinMode(PIN_BTN_DATA, INPUT);
    pinMode(PIN_BTN_CLK, OUTPUT);
    pinMode(PIN_BTN_LOAD, OUTPUT);
    digitalWrite(PIN_BTN_CLK, LOW);
    digitalWrite(PIN_BTN_LOAD, LOW);

    lastRaw = readRaw();
    printStartup();
}

void loop() {
    handleSerialInput();

    uint16_t raw = readRaw();

    if (raw != lastRaw) {
        // Finde welches Bit sich geaendert hat
        uint16_t changed = raw ^ lastRaw;

        Serial.print("Jetzt:  ");
        printBits(raw);

        // Zeige welche Bits sich geaendert haben
        Serial.print("  -> Bit ");
        for (int8_t i = 15; i >= 0; i--) {
            if ((changed >> i) & 1) {
                Serial.print(i);
                if ((raw >> i) & 1) {
                    Serial.print(" HIGH (losgelassen)");
                } else {
                    Serial.print(" LOW (GEDRUECKT)");
                }
                Serial.print(" ");
            }
        }
        Serial.println();

        lastRaw = raw;
    }

    // WICHTIG: Watchdog füttern - ESP32-S3 USB-CDC braucht das!
    delay(1);
}
