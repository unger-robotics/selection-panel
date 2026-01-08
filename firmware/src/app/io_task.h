// src/app/io_task.h
#ifndef IO_TASK_H
#define IO_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// =============================================================================
// IO Task
// =============================================================================
//
// Verantwortung:
// - Periodisches Einlesen der Taster (CD4021B)
// - Entprellen der Rohwerte
// - Auswahl aktualisieren (One-Hot)
// - LED-Zustand schreiben (74HC595)
// - Log-Events in Queue senden
// - LED-Befehle vom Pi verarbeiten (über Callback)
//
// Warum eigener Task?
// → vTaskDelayUntil() garantiert konstante 5 ms Periode
// → Unabhängig von Serial-Ausgabe (die blockieren kann)
//
// =============================================================================

// Startet den IO-Task auf CORE_APP mit PRIO_IO
void start_io_task(QueueHandle_t logQueue);

#endif // IO_TASK_H
