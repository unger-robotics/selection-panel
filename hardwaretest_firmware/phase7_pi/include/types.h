#ifndef TYPES_H
#define TYPES_H

#include "config.h"
#include <Arduino.h>

// =============================================================================
// Gemeinsame Datentypen
// =============================================================================

// LogEvent: Snapshot des Systemzustands für die Queue zwischen IO und Serial.
// Warum struct statt einzelne Werte?
// → Ein Queue-Element = ein atomarer Snapshot
// → Keine Race-Conditions zwischen Feldern
struct LogEvent {
    uint32_t ms;                    // Zeitstempel (für Debugging)
    uint8_t raw[BTN_BYTES];         // Rohzustand der Taster
    uint8_t deb[BTN_BYTES];         // Entprellter Zustand
    uint8_t led[LED_BYTES];         // LED-Ausgabezustand
    uint8_t activeId;               // Aktive Auswahl (0 = keine, 1-10 = ID)
    bool rawChanged;                // Flag: Raw hat sich geändert
    bool debChanged;                // Flag: Debounced hat sich geändert
    bool activeChanged;             // Flag: Auswahl hat sich geändert
};

#endif // TYPES_H
