#ifndef SERIAL_TASK_H
#define SERIAL_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// =============================================================================
// Serial Task
// =============================================================================
//
// Verantwortung:
// - Empfängt Log-Events aus Queue
// - Formatiert und gibt über Serial aus
// - Einzige Stelle die Serial.print() aufruft
//
// Warum eigener Task?
// → USB-CDC kann blockieren (10-100 ms möglich)
// → Ohne Trennung würde IO-Timing leiden
// → Queue entkoppelt Produzent (IO) und Konsument (Serial)
//
// =============================================================================

// Startet den Serial-Task auf CORE_APP mit PRIO_SERIAL
void start_serial_task(QueueHandle_t logQueue);

#endif // SERIAL_TASK_H
