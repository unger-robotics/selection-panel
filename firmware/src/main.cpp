/**
 * @file main.cpp
 * @brief Selection Panel Entry Point (v2.5.0)
 *
 * Aufbau:
 * - setup() erstellt Queue und startet Tasks
 * - loop() ist leer (Tasks uebernehmen die Arbeit)
 *
 * Warum Queue in main.cpp und nicht in einem Task?
 * Queue muss vor beiden Tasks existieren.
 * main.cpp ist der natuerliche Ort fuer "Verbindung" zwischen Modulen.
 *
 * Serial-Initialisierung:
 * Erfolgt in serial_task.cpp (einzige Stelle fuer Serial I/O).
 * Hier nur delay() fuer USB-CDC Stabilisierung.
 */

// =============================================================================
// INCLUDES
// =============================================================================

#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "app/io_task.h"
#include "app/serial_task.h"

// =============================================================================
// MODUL-LOKALE VARIABLEN
// =============================================================================

static QueueHandle_t _log_queue = nullptr;

// =============================================================================
// ARDUINO ENTRY POINTS
// =============================================================================

/**
 * @brief Arduino Setup - Erstellt Queue und startet Tasks
 */
void setup() {
    // USB-CDC braucht Zeit zum Initialisieren
    // Serial.begin() erfolgt in serial_task.cpp
    delay(1500);

    // Queue fuer Log-Events erstellen
    // Groesse: LOG_QUEUE_LEN Events, jedes sizeof(log_event_t) Bytes
    _log_queue = xQueueCreate(LOG_QUEUE_LEN, sizeof(log_event_t));

    // Tasks starten
    // IO hoch: harter 200 Hz Zyklus, soll jitterarm bleiben
    // Serial niedriger: darf drosseln, darf IO nicht stoeren
    start_io_task(_log_queue);
    start_serial_task(_log_queue);
}

/**
 * @brief Arduino Loop - Leer, Tasks uebernehmen die Arbeit
 */
void loop() {
    // Tasks uebernehmen die Arbeit
    // vTaskDelay(portMAX_DELAY) spart CPU-Zeit
    vTaskDelay(portMAX_DELAY);
}
