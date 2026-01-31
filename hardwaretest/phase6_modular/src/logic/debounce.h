#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include "bitops.h"
#include "config.h"
#include <Arduino.h>

// =============================================================================
// Zeitbasiertes Debouncing
// =============================================================================
//
// Warum zeitbasiert statt Counter-basiert?
// → Unabhängig von Abtastrate (IO_PERIOD_MS kann variieren)
// → Jeder Taster hat eigenen Timer (schnelle Tastenfolgen möglich)
// → Einfach zu verstehen und debuggen
//
// Algorithmus:
// 1. Bei jeder Rohwert-Änderung: Timer zurücksetzen
// 2. Wenn Timer abgelaufen UND Rohwert != Debounced: Übernehmen
//
// =============================================================================

class Debouncer {
public:
    // Initialisiert interne Zustände
    void init();

    // Aktualisiert Debounce-Zustand, gibt true zurück wenn sich etwas geändert hat
    // raw:  Aktueller Rohzustand [BTN_BYTES]
    // deb:  Entprellter Zustand [BTN_BYTES] (wird modifiziert)
    bool update(uint32_t nowMs, const uint8_t* raw, uint8_t* deb);

private:
    uint8_t rawPrev_[BTN_BYTES];       // Rohzustand vom letzten Zyklus
    uint32_t lastChange_[BTN_COUNT];   // Zeitpunkt der letzten Änderung pro Taster
};

#endif // DEBOUNCE_H
