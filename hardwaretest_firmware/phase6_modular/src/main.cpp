#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app/io_task.h"
#include "app/serial_task.h"

// =============================================================================
// SELECTION PANEL: Entry Point
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
// =============================================================================

static QueueHandle_t logQueue_ = nullptr;

void setup() {
    Serial.begin(115200);
    delay(1500); // USB-CDC braucht Zeit zum Initialisieren

    // Queue für Log-Events erstellen
    // Größe: LOG_QUEUE_LEN Events, jedes sizeof(LogEvent) Bytes
    logQueue_ = xQueueCreate(LOG_QUEUE_LEN, sizeof(LogEvent));

    // Tasks starten (beide auf Core 1)
    // Serial-Task zuerst, damit Header ausgegeben wird bevor IO startet
    start_serial_task(logQueue_);
    start_io_task(logQueue_);
}

void loop() {
    // Tasks übernehmen die Arbeit
    // vTaskDelay(portMAX_DELAY) spart CPU-Zeit
    vTaskDelay(portMAX_DELAY);
}
