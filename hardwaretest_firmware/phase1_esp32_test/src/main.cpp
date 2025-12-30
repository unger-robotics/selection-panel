/**
 * =============================================================================
 * Phase 1: ESP32-S3 XIAO Basistest
 * =============================================================================
 *
 * Ziel: Verifizieren, dass der ESP32-S3 XIAO funktioniert
 *
 * Test 1: Onboard-LED blinkt (zeigt: MCU läuft)
 * Test 2: Serial Echo (zeigt: USB-Kommunikation funktioniert)
 *
 * Erwartetes Verhalten:
 * - LED blinkt im 500ms-Takt
 * - Serial Monitor zeigt "READY" nach Start
 * - Eingegebener Text wird zurückgesendet (Echo)
 *
 * Hardware: Nur ESP32-S3 XIAO, kein weiteres Zubehör
 * =============================================================================
 */

#include <Arduino.h>

// XIAO ESP32-S3 hat eine User-LED auf GPIO21
// Falls nicht vorhanden, nutzen wir D0 als Ausgang für externe LED
constexpr uint8_t LED_BUILTIN_PIN = 21;

// Timing
constexpr uint32_t BLINK_INTERVAL_MS = 500;
constexpr uint32_t SERIAL_BAUD = 115200;

// Status
bool ledState = false;
uint32_t lastBlinkTime = 0;
uint32_t messageCount = 0;

void printStartup() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 1: ESP32-S3 XIAO Basistest");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Hardware-Info:");
    Serial.printf("  Chip: %s\n", ESP.getChipModel());
    Serial.printf("  Revision: %d\n", ESP.getChipRevision());
    Serial.printf("  Cores: %d\n", ESP.getChipCores());
    Serial.printf("  CPU Freq: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("  Flash: %d KB\n", ESP.getFlashChipSize() / 1024);
    Serial.printf("  Free Heap: %d Bytes\n", ESP.getFreeHeap());
    Serial.println();
    Serial.println("Tests:");
    Serial.println("  [1] LED blinkt alle 500ms");
    Serial.println("  [2] Serial Echo - Text eingeben und Enter");
    Serial.println();
    Serial.println("Befehle:");
    Serial.println("  PING  -> Antwortet mit PONG");
    Serial.println("  INFO  -> Zeigt Systeminfo");
    Serial.println("  HELLO -> Zeigt Startup-Nachricht");
    Serial.println("  HELP  -> Zeigt diese Hilfe");
    Serial.println();
    Serial.println("READY");
    Serial.println();
}

void setup() {
    // Serial initialisieren
    Serial.begin(SERIAL_BAUD);

    // Warten auf Serial-Verbindung (max 3 Sekunden)
    uint32_t startTime = millis();
    while (!Serial && (millis() - startTime < 3000)) {
        delay(10);
    }

    // LED-Pin als Ausgang
    pinMode(LED_BUILTIN_PIN, OUTPUT);
    digitalWrite(LED_BUILTIN_PIN, LOW);

    // Startup-Nachricht
    printStartup();
}

void handleSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.length() == 0)
            return;

        messageCount++;

        // Befehle verarbeiten
        if (input.equalsIgnoreCase("PING")) {
            Serial.println("PONG");
        } else if (input.equalsIgnoreCase("INFO")) {
            Serial.println();
            Serial.printf("Uptime: %lu ms\n", millis());
            Serial.printf("Free Heap: %d Bytes\n", ESP.getFreeHeap());
            Serial.printf("Messages: %lu\n", messageCount);
            Serial.printf("LED State: %s\n", ledState ? "ON" : "OFF");
            Serial.println();
        } else if (input.equalsIgnoreCase("HELP")) {
            Serial.println();
            Serial.println("Befehle: PING, INFO, HELLO, HELP");
            Serial.println("Oder beliebigen Text eingeben -> Echo");
            Serial.println();
        } else if (input.equalsIgnoreCase("HELLO")) {
            printStartup();
        } else {
            // Echo
            Serial.printf("Echo [%lu]: %s\n", messageCount, input.c_str());
        }
    }
}

void updateBlink() {
    uint32_t now = millis();

    if (now - lastBlinkTime >= BLINK_INTERVAL_MS) {
        lastBlinkTime = now;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN_PIN, ledState ? HIGH : LOW);
    }
}

void loop() {
    updateBlink();
    handleSerialInput();

    // WICHTIG: Watchdog füttern - ESP32-S3 USB-CDC braucht das!
    delay(1);
}
