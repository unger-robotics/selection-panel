// src/app/serial_task.cpp
#include "app/serial_task.h"

#include "bitops.h"
#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// =============================================================================
// Serial Task Implementation
// =============================================================================

static QueueHandle_t logQueue_ = nullptr;
static LedControlCallback ledCallback_ = nullptr;

// Letzter aktiver Button (für RELEASE-Erkennung)
static uint8_t lastActiveId_ = 0;

// Serial-Eingabepuffer
static char rxBuffer_[64];
static size_t rxIndex_ = 0;

// =============================================================================
// Debug Helpers (nur wenn SERIAL_PROTOCOL_ONLY == false)
// =============================================================================

static void printBinary(uint8_t value) {
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 1u);
    }
}

static void printByteArray(const char *label, const uint8_t *arr, size_t bytes) {
    Serial.print(label);
    for (size_t i = 0; i < bytes; ++i) {
        if (i) Serial.print(" | ");
        Serial.printf("IC%d: ", (int)i);
        printBinary(arr[i]);
        Serial.printf(" (0x%02X)", arr[i]);
    }
    Serial.println();
}

static void printPressedList(const uint8_t *deb) {
    Serial.print("Pressed: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (activeLow_pressed(deb, id)) {
            Serial.printf("%u ", id);
            any = true;
        }
    }
    if (!any) Serial.print('-');
    Serial.println();
}

static void printButtonsVerbose(const uint8_t *raw, const uint8_t *deb) {
    Serial.println("Buttons per ID (RAW/DEB)  [pressed=1 | released=0]");
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        Serial.printf("  T%02u  IC%u b%u   RAW=%u  DEB=%u\n", id,
                      (unsigned)btn_byte(id), (unsigned)btn_bit(id),
                      activeLow_pressed(raw, id) ? 1u : 0u,
                      activeLow_pressed(deb, id) ? 1u : 0u);
    }
}

static void printLEDsVerbose(const uint8_t *led) {
    Serial.println("LEDs per ID (STATE)  [on=1 | off=0]");
    for (uint8_t id = 1; id <= LED_COUNT; ++id) {
        Serial.printf("  LED%02u  IC%u b%u   STATE=%u\n", id,
                      (unsigned)led_byte(id), (unsigned)led_bit(id),
                      led_on(led, id) ? 1u : 0u);
    }
}

// =============================================================================
// Protokoll-Funktionen (ESP32 → Pi)
// =============================================================================

static void sendPong() {
    Serial.println("PONG");
    Serial.flush();
}

static void sendOk() {
    Serial.println("OK");
    Serial.flush();
}

static void sendError(const char* msg) {
    Serial.printf("ERROR %s\n", msg);
    Serial.flush();
}

static void sendVersion() {
    Serial.println("FW selection-panel v2.5.0");
    Serial.flush();
}

static void sendHelp() {
    Serial.println("Commands: PING, STATUS, VERSION, HELP");
    Serial.println("          LEDSET n, LEDON n, LEDOFF n, LEDCLR, LEDALL");
    Serial.flush();
}

static void sendStatus() {
    Serial.printf("CURLED %u\n", lastActiveId_);
    Serial.printf("BTNS %u\n", BTN_COUNT);
    Serial.printf("LEDS %u\n", LED_COUNT);
    Serial.printf("HEAP %u\n", ESP.getFreeHeap());
    Serial.printf("MODE %s\n", BTN_COUNT <= 10 ? "PROTOTYPE" : "PRODUCTION");
    sendOk();
}

// =============================================================================
// Befehlsverarbeitung (Pi → ESP32)
// =============================================================================

// Parst eine ID (001-100) aus einem String
static int parseId(const char* str) {
    while (*str == ' ') str++;  // Leerzeichen überspringen
    int id = atoi(str);
    if (id >= 1 && id <= LED_COUNT) {
        return id;
    }
    return -1;
}

static void processCommand(const char* cmd) {
    if (cmd[0] == '\0') return;  // Leerzeilen ignorieren
    
    // --- Einfache Befehle ---
    if (strcmp(cmd, "PING") == 0) {
        sendPong();
        return;
    }
    
    if (strcmp(cmd, "VERSION") == 0) {
        sendVersion();
        return;
    }
    
    if (strcmp(cmd, "HELP") == 0) {
        sendHelp();
        return;
    }
    
    if (strcmp(cmd, "STATUS") == 0) {
        sendStatus();
        return;
    }
    
    // --- LED-Befehle ohne ID ---
    if (strcmp(cmd, "LEDCLR") == 0) {
        if (ledCallback_) {
            ledCallback_(LED_CMD_CLEAR, 0);
            lastActiveId_ = 0;
        }
        sendOk();
        return;
    }
    
    if (strcmp(cmd, "LEDALL") == 0) {
        if (ledCallback_) {
            ledCallback_(LED_CMD_ALL, 0);
        }
        sendOk();
        return;
    }
    
    // --- LED-Befehle mit ID ---
    if (strncmp(cmd, "LEDSET ", 7) == 0) {
        int id = parseId(cmd + 7);
        if (id > 0) {
            if (ledCallback_) {
                ledCallback_(LED_CMD_SET, (uint8_t)id);
                lastActiveId_ = (uint8_t)id;
            }
            sendOk();
        } else {
            sendError("INVALID_ID");
        }
        return;
    }
    
    if (strncmp(cmd, "LEDON ", 6) == 0) {
        int id = parseId(cmd + 6);
        if (id > 0) {
            if (ledCallback_) {
                ledCallback_(LED_CMD_ON, (uint8_t)id);
            }
            sendOk();
        } else {
            sendError("INVALID_ID");
        }
        return;
    }
    
    if (strncmp(cmd, "LEDOFF ", 7) == 0) {
        int id = parseId(cmd + 7);
        if (id > 0) {
            if (ledCallback_) {
                ledCallback_(LED_CMD_OFF, (uint8_t)id);
            }
            sendOk();
        } else {
            sendError("INVALID_ID");
        }
        return;
    }
    
    // Unbekannter Befehl
    sendError("UNKNOWN_CMD");
}

// =============================================================================
// Serial-Eingabe lesen (non-blocking)
// =============================================================================

static void readSerialInput() {
    while (Serial.available()) {
        char c = Serial.read();
        
        // Zeilenende erkannt
        if (c == '\n' || c == '\r') {
            if (rxIndex_ > 0) {
                rxBuffer_[rxIndex_] = '\0';
                processCommand(rxBuffer_);
                rxIndex_ = 0;
            }
        }
        // Zeichen zum Puffer hinzufügen
        else if (rxIndex_ < sizeof(rxBuffer_) - 1) {
            rxBuffer_[rxIndex_++] = c;
        }
        // Puffer voll → verwerfen und neu starten
        else {
            rxIndex_ = 0;
        }
    }
}

// =============================================================================
// Task-Funktion
// =============================================================================

static void serialTaskFunction(void *) {
    Serial.begin(SERIAL_BAUD);
    delay(50);

    if (SERIAL_PROTOCOL_ONLY) {
        // Protokoll-Modus: Nur READY und FW senden
        if (SERIAL_SEND_READY) Serial.println("READY");
        if (SERIAL_SEND_FW_LINE) Serial.println("FW selection-panel v2.5.0");
    } else {
        // Debug-Modus: Ausführlicher Header
        Serial.println();
        Serial.println("========================================");
        Serial.println("Selection Panel v2.5.0");
        Serial.println("========================================");
        Serial.printf("BTN_COUNT:       %u\n", BTN_COUNT);
        Serial.printf("LED_COUNT:       %u\n", LED_COUNT);
        Serial.printf("IO_PERIOD_MS:    %u\n", IO_PERIOD_MS);
        Serial.printf("DEBOUNCE_MS:     %u\n", DEBOUNCE_MS);
        Serial.printf("LATCH_SELECTION: %s\n", LATCH_SELECTION ? "true" : "false");
        Serial.println("========================================");
        Serial.println("READY");
    }

    LogEvent event = {};

    for (;;) {
        // 1) Serial-Eingabe prüfen (Befehle vom Pi)
        readSerialInput();

        // 2) Queue mit Timeout lesen
        if (xQueueReceive(logQueue_, &event, pdMS_TO_TICKS(10)) == pdTRUE) {

            if (SERIAL_PROTOCOL_ONLY) {
                // --- Protokoll-Modus: Nur PRESS/RELEASE senden ---
                if (event.activeChanged) {
                    if (event.activeId > 0 && event.activeId <= BTN_COUNT) {
                        // Neuer Button aktiv → PRESS senden
                        Serial.printf("PRESS %03u\n", event.activeId);
                        Serial.flush();
                        lastActiveId_ = event.activeId;
                    } else if (lastActiveId_ > 0) {
                        // Kein Button mehr aktiv → RELEASE senden
                        Serial.printf("RELEASE %03u\n", lastActiveId_);
                        Serial.flush();
                        lastActiveId_ = 0;
                    }
                }
                continue;
            }

            // --- Debug-Modus: Ausführliche Ausgabe ---
            if (event.activeChanged) {
                if (event.activeId > 0) {
                    Serial.printf(">>> PRESS %03u\n", event.activeId);
                    lastActiveId_ = event.activeId;
                } else if (lastActiveId_ > 0) {
                    Serial.printf(">>> RELEASE %03u\n", lastActiveId_);
                    lastActiveId_ = 0;
                }
            }
            
            Serial.println("---");
            printByteArray("BTN RAW:    ", event.raw, BTN_BYTES);
            printByteArray("BTN DEB:    ", event.deb, BTN_BYTES);
            Serial.printf("Active LED (One-Hot): %u\n", event.activeId);
            printByteArray("LED STATE:  ", event.led, LED_BYTES);
            printPressedList(event.deb);

            if (LOG_VERBOSE_PER_ID) {
                printButtonsVerbose(event.raw, event.deb);
                printLEDsVerbose(event.led);
            }

            vTaskDelay(1);
        }
    }
}

// =============================================================================
// Public API
// =============================================================================

void start_serial_task(QueueHandle_t logQueue) {
    logQueue_ = logQueue;
    xTaskCreatePinnedToCore(serialTaskFunction, "Serial", 8192, nullptr,
                            PRIO_SERIAL, nullptr, CORE_APP);
}

void set_led_callback(LedControlCallback callback) {
    ledCallback_ = callback;
}
