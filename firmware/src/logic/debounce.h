/**
 * @file debounce.h
 * @brief Zeitbasiertes Debouncing fuer Taster
 *
 * Warum zeitbasiert statt Counter-basiert?
 * - Unabhaengig von Abtastrate (IO_PERIOD_MS kann variieren)
 * - Jeder Taster hat eigenen Timer (schnelle Tastenfolgen moeglich)
 * - Einfach zu verstehen und debuggen
 *
 * Algorithmus:
 * 1. Bei jeder Rohwert-Aenderung: Timer zuruecksetzen
 * 2. Wenn Timer abgelaufen UND Rohwert != Debounced: Uebernehmen
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

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
 * @brief Zeitbasierter Entpreller fuer Taster
 */
class Debouncer {
public:
    /**
     * @brief Konstruktor - initialisiert Member auf sichere Werte
     */
    Debouncer() : _raw_prev{}, _last_change{} {}

    /**
     * @brief Initialisiert interne Zustaende fuer Betrieb
     */
    void init();

    /**
     * @brief Aktualisiert Debounce-Zustand
     * @param now_ms Aktuelle Zeit in Millisekunden
     * @param raw Aktueller Rohzustand [BTN_BYTES]
     * @param deb Entprellter Zustand [BTN_BYTES] (wird modifiziert)
     * @return true wenn sich etwas geaendert hat
     */
    bool update(uint32_t now_ms, const uint8_t* raw, uint8_t* deb);

private:
    uint8_t _raw_prev[BTN_BYTES];       /**< Rohzustand vom letzten Zyklus */
    uint32_t _last_change[BTN_COUNT];   /**< Zeitpunkt der letzten Aenderung pro Taster */
};

#endif // DEBOUNCE_H
