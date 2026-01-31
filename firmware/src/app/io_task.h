/**
 * @file io_task.h
 * @brief IO Task fuer Taster-Einlesen und LED-Steuerung
 *
 * Verantwortung:
 * - Periodisches Einlesen der Taster (CD4021B)
 * - Entprellen der Rohwerte
 * - Auswahl aktualisieren (One-Hot)
 * - LED-Zustand schreiben (74HC595)
 * - Log-Events in Queue senden
 * - LED-Befehle vom Pi verarbeiten (ueber Callback)
 *
 * Warum eigener Task?
 * vTaskDelayUntil() garantiert konstante 5 ms Periode.
 * Unabhaengig von Serial-Ausgabe (die blockieren kann).
 */
#ifndef IO_TASK_H
#define IO_TASK_H

// =============================================================================
// INCLUDES
// =============================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

/**
 * @brief Startet den IO-Task auf CORE_APP mit PRIO_IO
 * @param log_queue Queue fuer Log-Events
 */
void start_io_task(QueueHandle_t log_queue);

#endif // IO_TASK_H
