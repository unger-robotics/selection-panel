#include "app/serial_task.h"

#include "bitops.h"
#include "config.h"
#include "types.h"
#include <Arduino.h>

// =============================================================================
// Serial Task Implementation
// =============================================================================

static QueueHandle_t logQueue_ = nullptr;

// -----------------------------------------------------------------------------
// Formatierungs-Hilfsfunktionen
// -----------------------------------------------------------------------------

static void printBinary(uint8_t value) {
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 1u);
    }
}

static void printByteArray(const char *label, const uint8_t *arr,
                           size_t bytes) {
    Serial.print(label);
    for (size_t i = 0; i < bytes; ++i) {
        if (i)
            Serial.print(" | ");
        Serial.printf("IC%d: ", i);
        printBinary(arr[i]);
        Serial.printf(" (0x%02X)", arr[i]);
    }
    Serial.println();
}

static void printPressedList(const uint8_t *deb) {
    Serial.print("Pressed IDs: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (activeLow_pressed(deb, id)) {
            if (any)
                Serial.print(' ');
            Serial.print(id);
            any = true;
        }
    }
    if (!any)
        Serial.print('-');
    Serial.println();
}

static void printButtonsVerbose(const uint8_t *raw, const uint8_t *deb) {
    Serial.println("Buttons per ID (RAW/DEB)  [pressed=1 | released=0]");
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        Serial.printf("  T%02d  IC%d b%d   RAW=%d  DEB=%d\n", id, btn_byte(id),
                      btn_bit(id), activeLow_pressed(raw, id) ? 1 : 0,
                      activeLow_pressed(deb, id) ? 1 : 0);
    }
}

static void printLEDsVerbose(const uint8_t *led) {
    Serial.println("LEDs per ID (state)       [on=1 | off=0]");
    for (uint8_t id = 1; id <= LED_COUNT; ++id) {
        const uint8_t byte = led_byte(id);
        const uint8_t bit = led_bit(id);
        const bool on = (led[byte] >> bit) & 1u;
        Serial.printf("  L%02d  IC%d b%d   %d\n", id, byte, bit, on ? 1 : 0);
    }
}

// -----------------------------------------------------------------------------
// Task-Funktion
// -----------------------------------------------------------------------------

static void serialTaskFunction(void *) {
    // Header einmal beim Start ausgeben
    Serial.println();
    Serial.println("========================================");
    Serial.println("Selection Panel: Modular + FreeRTOS");
    Serial.println("========================================");
    Serial.printf("IO_PERIOD_MS:    %u\n", IO_PERIOD_MS);
    Serial.printf("DEBOUNCE_MS:     %u\n", DEBOUNCE_MS);
    Serial.printf("LATCH_SELECTION: %s\n", LATCH_SELECTION ? "true" : "false");
    Serial.println("---");

    LogEvent event;

    for (;;) {
        // Blockiert bis Event in Queue (portMAX_DELAY = unendlich warten)
        if (xQueueReceive(logQueue_, &event, portMAX_DELAY) == pdTRUE) {
            Serial.println("---");
            printByteArray("BTN RAW:    ", event.raw, BTN_BYTES);
            printByteArray("BTN DEB:    ", event.deb, BTN_BYTES);
            Serial.printf("Active LED (One-Hot): %d\n", event.activeId);
            printByteArray("LED STATE:  ", event.led, LED_BYTES);
            printPressedList(event.deb);

            if (LOG_VERBOSE_PER_ID) {
                printButtonsVerbose(event.raw, event.deb);
                printLEDsVerbose(event.led);
            }

            // Kurz abgeben damit andere Tasks laufen können
            vTaskDelay(1);
        }
    }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void start_serial_task(QueueHandle_t logQueue) {
    logQueue_ = logQueue;

    xTaskCreatePinnedToCore(serialTaskFunction, "Serial", 8192, nullptr,
                            PRIO_SERIAL, nullptr, CORE_APP);
}
