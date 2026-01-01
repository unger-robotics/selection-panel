/**
 * @file main.cpp
 * @brief Button Panel Controller - ESP32-S3 XIAO (Dual-Core)
 * @version 2.4.1
 * @date 2025-01-01
 *
 * CHANGELOG v2.4.1:
 * - LED reagiert sofort bei Tastendruck (< 1ms, ohne Pi-Roundtrip)
 * - Pi wird parallel informiert (PRESS xxx)
 * - LEDSET vom Pi wird ignoriert wenn bereits korrekt gesetzt
 *
 * Architektur:
 * +-------------------------------------------------------------+
 * | Core 0: buttonTask                                          |
 * | - Button-Scanning alle 10ms                                 |
 * | - Debouncing                                                |
 * | - Events in Queue schreiben                                 |
 * +-------------------------------------------------------------+
 *                          | buttonEventQueue
 *                          v
 * +-------------------------------------------------------------+
 * | Core 1: Arduino loop()                                      |
 * | - Serial-Kommunikation                                      |
 * | - LED-Steuerung                                             |
 * | - Events aus Queue lesen                                    |
 * +-------------------------------------------------------------+
 *
 * Serial-Protokoll (115200 Baud):
 *
 * ESP32 -> Raspberry Pi:
 *   PRESS xxx    - Button wurde gedrueckt (001-100, 1-basiert!)
 *   RELEASE xxx  - Button wurde losgelassen
 *   READY        - Controller ist bereit
 *   OK           - Befehl ausgefuehrt
 *   ERROR msg    - Fehler aufgetreten
 *
 * Raspberry Pi -> ESP32:
 *   LEDSET xxx   - LED einschalten (one-hot, 1-basiert)
 *   LEDON xxx    - LED einschalten (additiv)
 *   LEDOFF xxx   - LED ausschalten
 *   LEDCLR       - Alle LEDs aus
 *   LEDALL       - Alle LEDs ein
 *   PING         - Verbindungstest
 *   STATUS       - Zustand abfragen
 *   TEST         - LED-Test (non-blocking)
 *   HELLO        - Startup-Nachricht anzeigen
 *
 * WICHTIG: Serial.flush() nach jedem Event für ESP32-S3 USB-CDC!
 */

#include "config.h"
#include "shift_register.h"
#include <Arduino.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// =============================================================================
// GLOBALE OBJEKTE
// =============================================================================

static OutputShiftRegister leds(PIN_595_DATA, PIN_595_CLOCK, PIN_595_LATCH,
                                NUM_595_CHIPS);
static InputShiftRegister buttons_hw(PIN_4021_DATA, PIN_4021_CLOCK,
                                     PIN_4021_LOAD, NUM_4021_CHIPS);
static ButtonManager buttons(buttons_hw, NUM_BUTTONS, BUTTON_ACTIVE_LOW);

static TaskHandle_t buttonTaskHandle = nullptr;
static QueueHandle_t buttonEventQueue = nullptr;

static String serialBuffer;

// LED-Test State Machine (non-blocking)
static struct {
    bool active;
    uint8_t currentLed;
    uint32_t lastStep;
} ledTest = {false, 0, 0};

// Queue-Overflow-Statistik
static struct {
    uint32_t overflowCount;
    uint32_t lastOverflowTime;
} queueStats = {0, 0};

// Aktuell gesetzte LED (fuer Optimierung: LEDSET ignorieren wenn schon korrekt)
static int8_t currentLedIndex = -1;  // -1 = keine LED aktiv, 0-99 = LED-Index

// =============================================================================
// STARTUP-NACHRICHT
// =============================================================================

static void printStartup() {
    Serial.println();
    Serial.println("========================================");
    Serial.print("Button Panel v2.4.1 (");
#ifdef PROTOTYPE_MODE
    Serial.print("PROTOTYPE: ");
#else
    Serial.print("PRODUCTION: ");
#endif
    Serial.print(NUM_LEDS);
    Serial.println(" LEDs)");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Pinbelegung:");
    Serial.println("  LED:    D0=Data, D1=Clock, D2=Latch");
    Serial.println("  Button: D3=Data, D4=Clock, D5=Load");
    Serial.println();
#ifdef PROTOTYPE_MODE
    if (USE_BUTTON_MAPPING) {
        Serial.println("Bit-Mapping: AKTIV (Prototyp)");
        Serial.print("  Taster 1-10 -> Bits: ");
        for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
            Serial.print(BUTTON_BIT_MAP[i]);
            if (i < NUM_BUTTONS - 1)
                Serial.print(",");
        }
        Serial.println();
    }
#else
    Serial.println("Bit-Mapping: LINEAR (Produktion)");
#endif
    Serial.println();
    Serial.println("NEU v2.4.1: LED reagiert sofort bei Tastendruck");
    Serial.println();
    Serial.println("Befehle:");
    Serial.println("  LEDSET n  - LED n ein (1-basiert, one-hot)");
    Serial.println("  LEDON n   - LED n ein (additiv)");
    Serial.println("  LEDOFF n  - LED n aus");
    Serial.println("  LEDCLR    - Alle LEDs aus");
    Serial.println("  LEDALL    - Alle LEDs ein");
    Serial.println("  TEST      - LED-Lauflicht");
    Serial.println("  STATUS    - Zustand anzeigen");
    Serial.println("  PING      - Verbindungstest");
    Serial.println("  HELLO     - Diese Nachricht");
    Serial.println();
    Serial.println("READY");
    Serial.println();
    Serial.flush();
}

// =============================================================================
// SERIAL-PROTOKOLL
// =============================================================================

static void sendReady() {
    Serial.println("READY");
    Serial.flush();
}

static void sendOK() {
    Serial.println("OK");
    Serial.flush();
}

static void sendError(const char *msg) {
    Serial.print("ERROR ");
    Serial.println(msg);
    Serial.flush();
}

static void sendButtonEvent(const ButtonEvent &event) {
    // 1-basierter Index (001-100)
    uint8_t displayIndex = event.index + 1;

    // Formatierter String in einem Stueck senden (verhindert Fragmentierung!)
    char buffer[16];
    if (event.type == ButtonEventType::PRESSED) {
        snprintf(buffer, sizeof(buffer), "PRESS %03d", displayIndex);
    } else {
        snprintf(buffer, sizeof(buffer), "RELEASE %03d", displayIndex);
    }

    Serial.println(buffer);
    Serial.flush();
}

static void sendStatus() {
    // LED-Zustand (nur genutzte LEDs)
    Serial.print("LEDS ");
    for (int8_t i = NUM_LEDS - 1; i >= 0; i--) {
        Serial.print(leds.getOutput(i) ? '1' : '0');
    }
    Serial.println();

    // Aktuelle LED
    Serial.print("CURLED ");
    Serial.println(currentLedIndex + 1);  // 1-basiert, 0 wenn keine

    // Button-Zustand
    Serial.print("BTNS ");
    uint32_t state = buttons.getFullState();
    for (int8_t i = NUM_BUTTONS - 1; i >= 0; i--) {
        Serial.print((state >> i) & 0x01);
    }
    Serial.println();

    // System-Info
    Serial.print("HEAP ");
    Serial.println(ESP.getFreeHeap());

    Serial.print("CORE ");
    Serial.println(xPortGetCoreID());

    // Queue-Info
    Serial.print("QFREE ");
    Serial.println(uxQueueSpacesAvailable(buttonEventQueue));

    // Overflow-Statistik
    if (queueStats.overflowCount > 0) {
        Serial.print("QOVFL ");
        Serial.println(queueStats.overflowCount);
    }

    // Build-Info
#ifdef PROTOTYPE_MODE
    Serial.println("MODE PROTOTYPE");
    if (USE_BUTTON_MAPPING) {
        Serial.println("MAPPING ON");
    }
#else
    Serial.println("MODE PRODUCTION");
#endif

    Serial.flush();
}

// =============================================================================
// BEFEHLSVERARBEITUNG
// =============================================================================

static int16_t extractNumber(const String &str, uint8_t startPos) {
    if (startPos >= str.length())
        return -1;

    String numStr = str.substring(startPos);
    numStr.trim();

    if (numStr.length() == 0)
        return -1;

    for (size_t i = 0; i < numStr.length(); i++) {
        if (!isDigit(numStr.charAt(i)))
            return -1;
    }

    return numStr.toInt();
}

static void processCommand(const String &command) {
    String cmd = command;
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.length() == 0)
        return;

#ifdef DEBUG_LEDS
    Serial.print("[CMD] ");
    Serial.println(cmd);
#endif

    // PING
    if (cmd == "PING") {
        Serial.println("PONG");
        Serial.flush();
        return;
    }

    // HELLO
    if (cmd == "HELLO") {
        printStartup();
        return;
    }

    // STATUS
    if (cmd == "STATUS") {
        sendStatus();
        return;
    }

    // LEDCLR
    if (cmd == "LEDCLR") {
        ledTest.active = false;
        leds.clear();
        leds.update();
        currentLedIndex = -1;
        sendOK();
        return;
    }

    // LEDALL
    if (cmd == "LEDALL") {
        ledTest.active = false;
        for (uint8_t i = 0; i < NUM_LEDS; i++) {
            leds.setOutput(i, true);
        }
        leds.update();
        currentLedIndex = -1;  // Nicht one-hot
        sendOK();
        return;
    }

    // LEDSET xxx (1-basiert!)
    if (cmd.startsWith("LEDSET ")) {
        int16_t num = extractNumber(cmd, 7);
        // Benutzer gibt 1-10 ein, intern 0-9
        if (num < 1 || num > NUM_LEDS) {
            sendError("Invalid LED (1-100)");
            return;
        }

        int8_t targetIndex = num - 1;  // Konvertiere zu 0-basiert

        // Optimierung: Wenn LED bereits gesetzt, nichts tun
        if (currentLedIndex == targetIndex) {
#ifdef DEBUG_LEDS
            Serial.print("[OPT] LED ");
            Serial.print(num);
            Serial.println(" bereits aktiv, uebersprungen");
            Serial.flush();
#endif
            sendOK();
            return;
        }

        ledTest.active = false;
        leds.setOnly(targetIndex);
        currentLedIndex = targetIndex;
        sendOK();
        return;
    }

    // LEDON xxx (1-basiert!)
    if (cmd.startsWith("LEDON ")) {
        int16_t num = extractNumber(cmd, 6);
        if (num < 1 || num > NUM_LEDS) {
            sendError("Invalid LED (1-100)");
            return;
        }
        ledTest.active = false;
        leds.setOutput(num - 1, true);
        leds.update();
        currentLedIndex = -1;  // Nicht mehr one-hot trackbar
        sendOK();
        return;
    }

    // LEDOFF xxx (1-basiert!)
    if (cmd.startsWith("LEDOFF ")) {
        int16_t num = extractNumber(cmd, 7);
        if (num < 1 || num > NUM_LEDS) {
            sendError("Invalid LED (1-100)");
            return;
        }
        leds.setOutput(num - 1, false);
        leds.update();
        if (currentLedIndex == num - 1) {
            currentLedIndex = -1;
        }
        sendOK();
        return;
    }

    // TEST
    if (cmd == "TEST") {
        Serial.println("Testing LEDs...");
        Serial.flush();
        ledTest.active = true;
        ledTest.currentLed = 0;
        ledTest.lastStep = millis();
        leds.setOnly(0);
        currentLedIndex = 0;
        return;
    }

    // STOP
    if (cmd == "STOP") {
        if (ledTest.active) {
            ledTest.active = false;
            leds.clear();
            leds.update();
            currentLedIndex = -1;
            Serial.println("Test stopped");
            Serial.flush();
        }
        sendOK();
        return;
    }

    // VERSION
    if (cmd == "VERSION") {
        Serial.print("FW 2.4.1 (");
#ifdef PROTOTYPE_MODE
        Serial.print("Proto ");
        Serial.print(NUM_LEDS);
#else
        Serial.print("Prod ");
        Serial.print(NUM_LEDS);
#endif
        Serial.println(" LEDs)");
        Serial.flush();
        sendOK();
        return;
    }

    // QRESET - Queue-Statistik zuruecksetzen
    if (cmd == "QRESET") {
        queueStats.overflowCount = 0;
        queueStats.lastOverflowTime = 0;
        Serial.println("Queue stats reset");
        Serial.flush();
        sendOK();
        return;
    }

    // HELP
    if (cmd == "HELP") {
        Serial.println();
        Serial.println("Befehle:");
        Serial.println("  LEDSET n  - LED n ein (1-basiert)");
        Serial.println("  LEDON n   - LED n ein (additiv)");
        Serial.println("  LEDOFF n  - LED n aus");
        Serial.println("  LEDCLR    - Alle aus");
        Serial.println("  LEDALL    - Alle ein");
        Serial.println("  TEST      - LED-Test");
        Serial.println("  STOP      - Test stoppen");
        Serial.println("  STATUS    - Zustand");
        Serial.println("  VERSION   - Firmware-Version");
        Serial.println("  HELLO     - Startup-Nachricht");
        Serial.println("  PING      - Verbindungstest");
        Serial.println();
        Serial.flush();
        return;
    }

    sendError("Unknown command");
}

static void handleSerial() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                processCommand(serialBuffer);
                serialBuffer = "";
            }
        } else if (serialBuffer.length() < MAX_COMMAND_LENGTH) {
            serialBuffer += c;
        }
    }
}

// =============================================================================
// LED-TEST STATE MACHINE
// =============================================================================

static void updateLedTest() {
    if (!ledTest.active)
        return;

    uint32_t now = millis();

    if ((now - ledTest.lastStep) >= LED_TEST_STEP_MS) {
        ledTest.lastStep = now;
        ledTest.currentLed++;

        if (ledTest.currentLed >= NUM_LEDS) {
            ledTest.active = false;
            leds.clear();
            leds.update();
            currentLedIndex = -1;
            Serial.println("Test complete");
            Serial.flush();
            sendOK();
        } else {
            leds.setOnly(ledTest.currentLed);
            currentLedIndex = ledTest.currentLed;
        }
    }
}

// =============================================================================
// BUTTON TASK (CORE 0)
// =============================================================================

static void buttonTask(void *parameter) {
    (void)parameter;

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t pollInterval = pdMS_TO_TICKS(BUTTON_POLL_MS);

    ButtonEvent event;

#ifdef DEBUG_TASKS
    Serial.print("[TASK] Button task started on Core ");
    Serial.println(xPortGetCoreID());
    Serial.flush();
#endif

    esp_task_wdt_add(nullptr);

    for (;;) {
        esp_task_wdt_reset();

        if (buttons.update(&event)) {
            if (buttonEventQueue != nullptr) {
                BaseType_t result = xQueueSend(buttonEventQueue, &event, 0);

                if (result != pdTRUE) {
                    queueStats.overflowCount++;
                    queueStats.lastOverflowTime = millis();

#ifdef DEBUG_QUEUE_OVERFLOW
                    Serial.print("[WARN] Queue overflow! Count: ");
                    Serial.println(queueStats.overflowCount);
                    Serial.flush();
#endif
                }
            }
        }

        vTaskDelayUntil(&lastWakeTime, pollInterval);
    }
}

// =============================================================================
// BUTTON EVENT HANDLER (CORE 1)
// =============================================================================

static void handleButtonEvents() {
    ButtonEvent event;

    while (xQueueReceive(buttonEventQueue, &event, 0) == pdTRUE) {
        // =====================================================================
        // NEU v2.4.1: LED SOFORT bei Tastendruck setzen (< 1ms Reaktionszeit)
        // =====================================================================
        if (event.type == ButtonEventType::PRESSED) {
            // LED-Test abbrechen wenn Taste gedrueckt
            ledTest.active = false;

            // LED sofort setzen - OHNE auf Pi zu warten!
            leds.setOnly(event.index);
            currentLedIndex = event.index;

#ifdef DEBUG_LEDS
            Serial.print("[FAST] LED ");
            Serial.print(event.index + 1);
            Serial.println(" sofort gesetzt");
            Serial.flush();
#endif
        }

        // Dann Event an Pi senden (parallel zur bereits leuchtenden LED)
        sendButtonEvent(event);
    }
}

// =============================================================================
// SETUP & LOOP
// =============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);

    uint32_t startTime = millis();
    while (!Serial && (millis() - startTime) < 3000) {
        delay(10);
    }

    serialBuffer.reserve(MAX_COMMAND_LENGTH + 1);

    // LED-Hardware
    if (!leds.begin()) {
        Serial.println("ERROR LED init failed");
        Serial.flush();
        while (1) {
            delay(1000);
        }
    }

    // Button-Manager
    if (!buttons.begin()) {
        Serial.println("ERROR Button init failed");
        Serial.flush();
        while (1) {
            delay(1000);
        }
    }

    // Event-Queue
    buttonEventQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(ButtonEvent));
    if (buttonEventQueue == nullptr) {
        Serial.println("ERROR Queue create failed");
        Serial.flush();
        while (1) {
            delay(1000);
        }
    }

    // Button-Task auf Core 0
    BaseType_t result = xTaskCreatePinnedToCore(
        buttonTask, "ButtonTask", TASK_STACK_BUTTONS, nullptr,
        TASK_PRIORITY_BUTTONS, &buttonTaskHandle, CORE_BUTTONS);

    if (result != pdPASS) {
        Serial.println("ERROR Task create failed");
        Serial.flush();
        while (1) {
            delay(1000);
        }
    }

    // Watchdog
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(nullptr);

    // Startup-Nachricht
    printStartup();

    // Startup-LED-Test
    ledTest.active = true;
    ledTest.currentLed = 0;
    ledTest.lastStep = millis();
    leds.setOnly(0);
    currentLedIndex = 0;

    Serial.println("Running startup LED test...");
    Serial.flush();

#ifdef DEBUG_TASKS
    Serial.print("[TASK] Main loop on Core ");
    Serial.println(xPortGetCoreID());
    Serial.flush();
#endif
}

void loop() {
    esp_task_wdt_reset();
    handleSerial();
    handleButtonEvents();
    updateLedTest();

    // READY nach Startup-Test (nur einmal)
    static bool readySent = false;
    if (!readySent && !ledTest.active) {
        sendReady();
        readySent = true;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
}
