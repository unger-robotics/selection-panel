/**
 * @file main.cpp
 * @brief Phase 1: Arduino-Modell (Single-Threaded)
 * 
 * Demonstriert das klassische Arduino-Pattern:
 * - setup() läuft einmal beim Start
 * - loop() läuft endlos auf Core 1
 * - Alles sequentiell, einfach zu verstehen
 * 
 * Wann dieses Modell wählen?
 * - Einfache Projekte ohne harte Echtzeit-Anforderungen
 * - Wenn alle Tasks in <10ms abgearbeitet werden können
 * - Prototyping und schnelle Tests
 */

#include <Arduino.h>

// =============================================================================
// Konfiguration
// =============================================================================

// Timing in Millisekunden
// Warum 500ms? Lesbare serielle Ausgabe ohne Scroll-Flut
constexpr uint32_t TASK_INTERVAL_MS = 500;

// =============================================================================
// Zustand
// =============================================================================

// Zähler simulieren Arbeit (in echtem Projekt: Sensorwerte, Button-States)
static uint32_t counterA = 0;
static uint32_t counterB = 0;

// Zeitstempel für nicht-blockierendes Timing
// Warum nicht delay()? Blockiert die CPU, andere Arbeit unmöglich
static uint32_t lastRunMs = 0;

// =============================================================================
// Tasks (als Funktionen)
// =============================================================================

/**
 * Simuliert Taster-Einlesen
 * In echtem Projekt: readButtons(), Sensor-Abfrage, etc.
 */
static void taskA() {
    counterA++;
    Serial.print("[Task A] Counter: ");
    Serial.print(counterA);
    Serial.print(" | Core: ");
    Serial.println(xPortGetCoreID());
}

/**
 * Simuliert LED-Ausgabe
 * In echtem Projekt: writeLEDs(), Display-Update, etc.
 */
static void taskB() {
    counterB += 2;
    Serial.print("[Task B] Counter: ");
    Serial.print(counterB);
    Serial.print(" | Core: ");
    Serial.println(xPortGetCoreID());
}

// =============================================================================
// Arduino Entry Points
// =============================================================================

void setup() {
    Serial.begin(115200);
    
    // Warte auf Serial (USB-CDC braucht Zeit)
    // Warum Timeout? Falls kein Monitor angeschlossen, nicht ewig warten
    while (!Serial && millis() < 2000) {}
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 1: Arduino-Modell (Single-Threaded)");
    Serial.println("========================================");
    Serial.println();
    Serial.println("Beide Tasks laufen sequentiell auf Core 1.");
    Serial.println("Task A -> Task B -> Warten -> Task A -> ...");
    Serial.println();
    Serial.print("Loop laeuft auf Core: ");
    Serial.println(xPortGetCoreID());
    Serial.println();
}

void loop() {
    // Nicht-blockierendes Timing
    // Warum? millis()-Pattern erlaubt andere Arbeit zwischen den Tasks
    uint32_t now = millis();
    
    if (now - lastRunMs >= TASK_INTERVAL_MS) {
        lastRunMs = now;
        
        // Sequentielle Ausführung: A fertig, dann B
        // Problem: Wenn A lange dauert, verzögert sich B
        taskA();
        taskB();
        
        Serial.println("---");
    }
    
    // Hier könnte weitere Arbeit passieren (z.B. Serial-Kommandos prüfen)
    // Aktuell: Busy Waiting (CPU läuft, tut aber nichts Nützliches)
}
