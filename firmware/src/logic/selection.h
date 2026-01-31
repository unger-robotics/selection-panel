/**
 * @file selection.h
 * @brief One-Hot Selection Logik
 *
 * Waehlt einen Taster aus wenn mehrere gedrueckt sind.
 *
 * Verhalten:
 * - "Last press wins": Neuer Tastendruck ueberschreibt vorherige Auswahl
 * - Flanken-basiert: Reagiert nur auf Uebergaenge (nicht gedrueckt -> gedrueckt)
 * - LATCH_SELECTION=true: Auswahl bleibt nach Loslassen bestehen
 * - LATCH_SELECTION=false: Auswahl erlischt wenn nichts gedrueckt
 */
#ifndef SELECTION_H
#define SELECTION_H

// =============================================================================
// INCLUDES
// =============================================================================

#include "bitops.h"
#include "config.h"
#include <Arduino.h>

// =============================================================================
// CLASSES
// =============================================================================

/**
 * @brief One-Hot Auswahllogik fuer Taster
 */
class Selection {
public:
    /**
     * @brief Konstruktor - initialisiert Member auf sichere Werte
     */
    Selection() : _deb_prev{} {}

    /**
     * @brief Initialisiert interne Zustaende fuer Betrieb
     */
    void init();

    /**
     * @brief Aktualisiert Auswahl basierend auf Flanken
     * @param deb_now Aktueller entprellter Zustand [BTN_BYTES]
     * @param active_id Aktuelle Auswahl (wird modifiziert)
     * @return true wenn sich active_id geaendert hat
     */
    bool update(const uint8_t* deb_now, uint8_t& active_id);

private:
    uint8_t _deb_prev[BTN_BYTES];  /**< Debounced-Zustand vom letzten Zyklus */
};

#endif // SELECTION_H
