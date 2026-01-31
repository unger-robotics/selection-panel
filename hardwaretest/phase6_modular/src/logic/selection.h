#ifndef SELECTION_H
#define SELECTION_H

#include "bitops.h"
#include "config.h"
#include <Arduino.h>

// =============================================================================
// One-Hot Selection Logik
// =============================================================================
//
// Aufgabe: Wählt einen Taster aus wenn mehrere gedrückt sind.
//
// Verhalten:
// - "Last press wins": Neuer Tastendruck überschreibt vorherige Auswahl
// - Flanken-basiert: Reagiert nur auf Übergänge (nicht gedrückt → gedrückt)
// - LATCH_SELECTION=true: Auswahl bleibt nach Loslassen bestehen
// - LATCH_SELECTION=false: Auswahl erlischt wenn nichts gedrückt
//
// =============================================================================

class Selection {
public:
    // Initialisiert interne Zustände
    void init();

    // Aktualisiert Auswahl basierend auf Flanken
    // debNow:   Aktueller entprellter Zustand [BTN_BYTES]
    // activeId: Aktuelle Auswahl (wird modifiziert)
    // Rückgabe: true wenn sich activeId geändert hat
    bool update(const uint8_t* debNow, uint8_t& activeId);

private:
    uint8_t debPrev_[BTN_BYTES];  // Debounced-Zustand vom letzten Zyklus
};

#endif // SELECTION_H
