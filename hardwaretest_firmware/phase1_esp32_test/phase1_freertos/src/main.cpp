/**
 * @file main.cpp
 * @brief Phase 1: FreeRTOS Multi-Tasking Demo
 * @version 2.0.0
 * 
 * Demonstriert FreeRTOS-Konzepte auf ESP32-S3:
 * - Message Queue (Producer-Consumer Pattern)
 * - Task-Prioritäten
 * - Präemptives Multitasking
 * 
 * Architektur-Entscheidung:
 * Der ESP32-S3 XIAO nutzt USB-CDC (natives USB) statt Hardware-UART.
 * Der TinyUSB-Stack läuft auf Core 0. Um Race Conditions zu vermeiden,
 * laufen alle User-Tasks auf Core 1 mit einem dedizierten Serial-Task.
 */

#include <Arduino.h>

// =============================================================================
// Konfiguration
// =============================================================================

constexpr uint32_t TASK_A_INTERVAL_MS = 600;  // Task A: langsamer
constexpr uint32_t TASK_B_INTERVAL_MS = 400;  // Task B: schneller → erscheint öfter
constexpr uint32_t STACK_SIZE = 4096;         // Stack pro Task (Bytes)
constexpr UBaseType_t TASK_PRIORITY = 1;      // Arbeits-Tasks: niedrige Priorität
constexpr UBaseType_t SERIAL_PRIORITY = 10;   // Serial-Task: höchste Priorität

// Queue-Konfiguration
constexpr size_t QUEUE_LENGTH = 20;           // Puffer für 20 Nachrichten
constexpr size_t MSG_MAX_LEN = 64;            // Max. Zeichen pro Nachricht

// =============================================================================
// Datenstrukturen
// =============================================================================

struct LogMessage {
    char text[MSG_MAX_LEN];
};

// =============================================================================
// Globaler Zustand
// =============================================================================

static uint32_t counterA = 0;
static uint32_t counterB = 0;
static QueueHandle_t logQueue = NULL;

// =============================================================================
// Logging-API
// =============================================================================

/**
 * Thread-sichere Log-Funktion.
 * 
 * Statt direkt Serial.print() aufzurufen, wird die Nachricht in eine
 * Queue geschoben. Der Serial-Task liest die Queue und gibt aus.
 * 
 * Vorteile:
 * - Kein Mutex nötig (Queue ist thread-sicher)
 * - Producer blockieren nicht (xQueueSend mit timeout=0)
 * - Nachrichten gehen nicht verloren (Puffer)
 */
static void logPrint(const char* taskName, uint32_t counter, uint8_t core) {
    LogMessage msg;
    snprintf(msg.text, sizeof(msg.text), "[%s] Counter: %lu | Core: %u", 
             taskName, (unsigned long)counter, core);
    
    xQueueSend(logQueue, &msg, 0);
}

// =============================================================================
// FreeRTOS Tasks
// =============================================================================

/**
 * Serial-Task: Consumer für Log-Nachrichten.
 * 
 * Läuft mit höchster Priorität (10), damit keine andere Task
 * während der Serial-Ausgabe unterbricht. Nach jeder Ausgabe
 * gibt vTaskDelay(1) die CPU kurz frei.
 */
static void taskSerial(void* param) {
    (void)param;
    LogMessage msg;
    
    while (true) {
        if (xQueueReceive(logQueue, &msg, portMAX_DELAY) == pdTRUE) {
            Serial.println(msg.text);
            Serial.flush();
            vTaskDelay(1);
        }
    }
}

/**
 * Task A: Producer mit 600ms Intervall.
 * 
 * vTaskDelay() gibt die CPU frei, im Gegensatz zu delay().
 * Während Task A wartet, können andere Tasks laufen.
 */
static void taskA(void* param) {
    (void)param;
    
    while (true) {
        counterA++;
        logPrint("Task A", counterA, xPortGetCoreID());
        vTaskDelay(pdMS_TO_TICKS(TASK_A_INTERVAL_MS));
    }
}

/**
 * Task B: Producer mit 400ms Intervall.
 * 
 * Schnelleres Intervall als Task A → erscheint ~1.5× öfter.
 * Zeigt dass Tasks unabhängig voneinander laufen.
 */
static void taskB(void* param) {
    (void)param;
    
    while (true) {
        counterB += 2;
        logPrint("Task B", counterB, xPortGetCoreID());
        vTaskDelay(pdMS_TO_TICKS(TASK_B_INTERVAL_MS));
    }
}

// =============================================================================
// Arduino Entry Points
// =============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 1: FreeRTOS Multi-Tasking Demo");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Architektur: Producer-Consumer mit Queue");
    Serial.println("Task A: 600ms | Task B: 400ms | Core: 1");
    Serial.println();
    
    // Queue erstellen (vor Task-Start)
    logQueue = xQueueCreate(QUEUE_LENGTH, sizeof(LogMessage));
    if (logQueue == NULL) {
        Serial.println("FEHLER: Queue-Erstellung fehlgeschlagen");
        while (true) {}
    }
    
    // Alle Tasks auf Core 1 (Core 0 = USB-CDC Stack)
    xTaskCreatePinnedToCore(taskSerial, "Serial", STACK_SIZE, NULL, SERIAL_PRIORITY, NULL, 1);
    xTaskCreatePinnedToCore(taskA, "TaskA", STACK_SIZE, NULL, TASK_PRIORITY, NULL, 1);
    xTaskCreatePinnedToCore(taskB, "TaskB", STACK_SIZE, NULL, TASK_PRIORITY, NULL, 1);
    
    Serial.println("Tasks gestartet.");
    Serial.println();
    Serial.flush();
    delay(100);
}

void loop() {
    // Leer: Arbeit passiert in den Tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
