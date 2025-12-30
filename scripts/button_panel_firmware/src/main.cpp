/**
 * @file main.cpp
 * @brief Button Panel Controller - ESP32-S3 XIAO (Dual-Core)
 * @version 2.2.0
 * @date 2025-12-26
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
 *   PRESS xxx    - Button wurde gedrueckt (000-099)
 *   RELEASE xxx  - Button wurde losgelassen
 *   READY        - Controller ist bereit
 *   OK           - Befehl ausgefuehrt
 *   ERROR msg    - Fehler aufgetreten
 *
 * Raspberry Pi -> ESP32:
 *   LEDSET xxx   - LED einschalten (one-hot)
 *   LEDON xxx    - LED einschalten (additiv)
 *   LEDOFF xxx   - LED ausschalten
 *   LEDCLR       - Alle LEDs aus
 *   LEDALL       - Alle LEDs ein
 *   PING         - Verbindungstest
 *   STATUS       - Zustand abfragen
 *   TEST         - LED-Test (non-blocking)
 */

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "shift_register.h"

// =============================================================================
// GLOBALE OBJEKTE
// =============================================================================

static OutputShiftRegister leds(PIN_595_DATA, PIN_595_CLOCK, PIN_595_LATCH, NUM_595_CHIPS);
static InputShiftRegister buttons_hw(PIN_4021_DATA, PIN_4021_CLOCK, PIN_4021_LOAD, NUM_4021_CHIPS);
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

// =============================================================================
// SERIAL-PROTOKOLL
// =============================================================================

static void sendReady() {
    Serial.println("READY");
}

static void sendOK() {
    Serial.println("OK");
}

static void sendError(const char* msg) {
    Serial.print("ERROR ");
    Serial.println(msg);
}

static void sendButtonEvent(const ButtonEvent& event) {
    const char* typeStr = (event.type == ButtonEventType::PRESSED) ? "PRESS" : "RELEASE";

    Serial.print(typeStr);
    Serial.print(' ');

    // Zero-padded Index (000-099)
    if (event.index < 10) {
        Serial.print("00");
    } else if (event.index < 100) {
        Serial.print('0');
    }
    Serial.println(event.index);
}

static void sendStatus() {
    // LED-Zustand (nur genutzte LEDs)
    Serial.print("LEDS ");
    for (int8_t i = NUM_LEDS - 1; i >= 0; i--) {
        Serial.print(leds.getOutput(i) ? '1' : '0');
    }
    Serial.println();

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
#else
    Serial.println("MODE PRODUCTION");
#endif
}

// =============================================================================
// BEFEHLSVERARBEITUNG
// =============================================================================

static int16_t extractNumber(const String& str, uint8_t startPos) {
    if (startPos >= str.length()) return -1;

    String numStr = str.substring(startPos);
    numStr.trim();

    if (numStr.length() == 0) return -1;

    for (size_t i = 0; i < numStr.length(); i++) {
        if (!isDigit(numStr.charAt(i))) return -1;
    }

    return numStr.toInt();
}

static void processCommand(const String& command) {
    String cmd = command;
    cmd.trim();
    cmd.toUpperCase();

    if (cmd.length() == 0) return;

#ifdef DEBUG_LEDS
    Serial.print("[CMD] ");
    Serial.println(cmd);
#endif

    // PING
    if (cmd == "PING") {
        Serial.println("PONG");
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
        sendOK();
        return;
    }

    // LEDSET xxx
    if (cmd.startsWith("LEDSET ")) {
        int16_t num = extractNumber(cmd, 7);
        if (num < 0 || num >= NUM_LEDS) {
            sendError("Invalid LED index");
            return;
        }
        ledTest.active = false;
        leds.setOnly(num);
        sendOK();
        return;
    }

    // LEDON xxx
    if (cmd.startsWith("LEDON ")) {
        int16_t num = extractNumber(cmd, 6);
        if (num < 0 || num >= NUM_LEDS) {
            sendError("Invalid LED index");
            return;
        }
        ledTest.active = false;
        leds.setOutput(num, true);
        leds.update();
        sendOK();
        return;
    }

    // LEDOFF xxx
    if (cmd.startsWith("LEDOFF ")) {
        int16_t num = extractNumber(cmd, 7);
        if (num < 0 || num >= NUM_LEDS) {
            sendError("Invalid LED index");
            return;
        }
        leds.setOutput(num, false);
        leds.update();
        sendOK();
        return;
    }

    // TEST
    if (cmd == "TEST") {
        Serial.println("Testing LEDs...");
        ledTest.active = true;
        ledTest.currentLed = 0;
        ledTest.lastStep = millis();
        leds.setOnly(0);
        return;
    }

    // STOP
    if (cmd == "STOP") {
        if (ledTest.active) {
            ledTest.active = false;
            leds.clear();
            leds.update();
            Serial.println("Test stopped");
        }
        sendOK();
        return;
    }

    // VERSION
    if (cmd == "VERSION") {
        Serial.print("FW 2.2.0 (");
#ifdef PROTOTYPE_MODE
        Serial.print("Proto ");
        Serial.print(NUM_LEDS);
#else
        Serial.print("Prod ");
        Serial.print(NUM_LEDS);
#endif
        Serial.println(" LEDs)");
        sendOK();
        return;
    }

    // QRESET - Queue-Statistik zuruecksetzen
    if (cmd == "QRESET") {
        queueStats.overflowCount = 0;
        queueStats.lastOverflowTime = 0;
        Serial.println("Queue stats reset");
        sendOK();
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
    if (!ledTest.active) return;

    uint32_t now = millis();

    if ((now - ledTest.lastStep) >= LED_TEST_STEP_MS) {
        ledTest.lastStep = now;
        ledTest.currentLed++;

        if (ledTest.currentLed >= NUM_LEDS) {
            ledTest.active = false;
            leds.clear();
            leds.update();
            Serial.println("Test complete");
            sendOK();
        } else {
            leds.setOnly(ledTest.currentLed);
        }
    }
}

// =============================================================================
// BUTTON TASK (CORE 0)
// =============================================================================

static void buttonTask(void* parameter) {
    (void)parameter;

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t pollInterval = pdMS_TO_TICKS(BUTTON_POLL_MS);

    ButtonEvent event;

#ifdef DEBUG_TASKS
    Serial.print("[TASK] Button task started on Core ");
    Serial.println(xPortGetCoreID());
#endif

    esp_task_wdt_add(nullptr);

    for (;;) {
        esp_task_wdt_reset();

        if (buttons.update(&event)) {
            if (buttonEventQueue != nullptr) {
                BaseType_t result = xQueueSend(buttonEventQueue, &event, 0);

                // Queue-Overflow-Handling
                if (result != pdTRUE) {
                    queueStats.overflowCount++;
                    queueStats.lastOverflowTime = millis();

#ifdef DEBUG_QUEUE_OVERFLOW
                    // Achtung: Serial aus ISR/Task ist nicht ideal
                    // Aber fuer Debug-Zwecke akzeptabel
                    Serial.print("[WARN] Queue overflow! Count: ");
                    Serial.println(queueStats.overflowCount);
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

    // Alle Events aus der Queue lesen (non-blocking)
    while (xQueueReceive(buttonEventQueue, &event, 0) == pdTRUE) {
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

    // Startup-Info
    Serial.println();
    Serial.println("================================");
    Serial.print("Button Panel v2.2.0 (");
#ifdef PROTOTYPE_MODE
    Serial.print("PROTOTYPE: ");
#else
    Serial.print("PRODUCTION: ");
#endif
    Serial.print(NUM_LEDS);
    Serial.println(" LEDs)");
    Serial.println("================================");

    // LED-Hardware
    if (!leds.begin()) {
        Serial.println("ERROR LED init failed");
        while (1) { delay(1000); }
    }
    Serial.print("LEDs: ");
    Serial.print(leds.getNumChips());
    Serial.println(" chips OK");

    // Button-Manager
    if (!buttons.begin()) {
        Serial.println("ERROR Button init failed");
        while (1) { delay(1000); }
    }
    Serial.print("Buttons: ");
    Serial.print(NUM_BUTTONS);
    Serial.println(" OK");

    // Event-Queue
    buttonEventQueue = xQueueCreate(BUTTON_QUEUE_SIZE, sizeof(ButtonEvent));
    if (buttonEventQueue == nullptr) {
        Serial.println("ERROR Queue create failed");
        while (1) { delay(1000); }
    }
    Serial.print("Queue: ");
    Serial.print(BUTTON_QUEUE_SIZE);
    Serial.println(" slots OK");

    // Button-Task auf Core 0
    BaseType_t result = xTaskCreatePinnedToCore(
        buttonTask,
        "ButtonTask",
        TASK_STACK_BUTTONS,
        nullptr,
        TASK_PRIORITY_BUTTONS,
        &buttonTaskHandle,
        CORE_BUTTONS
    );

    if (result != pdPASS) {
        Serial.println("ERROR Task create failed");
        while (1) { delay(1000); }
    }
    Serial.println("ButtonTask: Core 0 OK");

    // Watchdog
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(nullptr);
    Serial.print("Watchdog: ");
    Serial.print(WATCHDOG_TIMEOUT_MS / 1000);
    Serial.println("s OK");

    // Startup-LED-Test
    ledTest.active = true;
    ledTest.currentLed = 0;
    ledTest.lastStep = millis();
    leds.setOnly(0);

    Serial.println("--------------------------------");
    Serial.println("Running startup LED test...");

#ifdef DEBUG_TASKS
    Serial.print("[TASK] Main loop on Core ");
    Serial.println(xPortGetCoreID());
#endif
}

void loop() {
    esp_task_wdt_reset();
    handleSerial();
    handleButtonEvents();
    updateLedTest();

    // READY nach Startup-Test
    static bool readySent = false;
    if (!readySent && !ledTest.active) {
        sendReady();
        readySent = true;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
}
