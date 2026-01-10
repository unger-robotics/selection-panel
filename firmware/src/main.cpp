// src/main.cpp
#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app/io_task.h"
#include "app/serial_task.h"

// =============================================================================
// SELECTION PANEL: Entry Point (v2.5.0)
// =============================================================================
//
// Aufbau:
// - setup() erstellt Queue und startet Tasks
// - loop() ist leer (Tasks übernehmen die Arbeit)
//
// Warum Queue in main.cpp und nicht in einem Task?
// → Queue muss vor beiden Tasks existieren
// → main.cpp ist der natürliche Ort für "Verbindung" zwischen Modulen
//
// Serial-Initialisierung:
// → Erfolgt in serial_task.cpp (einzige Stelle für Serial I/O)
// → Hier nur delay() für USB-CDC Stabilisierung
//
// =============================================================================

static QueueHandle_t logQueue_ = nullptr;

void setup() {
    // USB-CDC braucht Zeit zum Initialisieren
    // Serial.begin() erfolgt in serial_task.cpp
    delay(1500);

    // Queue für Log-Events erstellen
    // Größe: LOG_QUEUE_LEN Events, jedes sizeof(LogEvent) Bytes
    logQueue_ = xQueueCreate(LOG_QUEUE_LEN, sizeof(LogEvent));

    // IO hoch: harter 200 Hz Zyklus, soll jitterarm bleiben
    // Serial niedriger: darf drosseln, darf IO nicht stören
    constexpr UBaseType_t PRIO_IO = 5;
    constexpr UBaseType_t PRIO_SERIAL = 2;
}

void loop() {
    // Tasks übernehmen die Arbeit
    // vTaskDelay(portMAX_DELAY) spart CPU-Zeit
    vTaskDelay(portMAX_DELAY);
}
