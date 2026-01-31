/**
 * @file debounce.cpp
 * @brief Debouncer Implementation
 */

// =============================================================================
// INCLUDES
// =============================================================================

#include "logic/debounce.h"

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

void Debouncer::init() {
    // Alle Taster als "losgelassen" initialisieren (0xFF = Active-Low)
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        _raw_prev[i] = 0xFF;
    }

    // Timer auf 0 = sofortige Uebernahme beim ersten echten Tastendruck
    for (size_t i = 0; i < BTN_COUNT; ++i) {
        _last_change[i] = 0;
    }
}

bool Debouncer::update(uint32_t now_ms, const uint8_t *raw, uint8_t *deb) {
    bool any_changed = false;

    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool raw_now = activeLow_pressed(raw, id);
        const bool raw_prev = activeLow_pressed(_raw_prev, id);
        const bool deb_now = activeLow_pressed(deb, id);

        // Rohwert geaendert? -> Timer zuruecksetzen
        // Das ist der "Prellen erkannt"-Moment
        if (raw_now != raw_prev) {
            _last_change[id - 1] = now_ms;
        }

        // Timer abgelaufen UND Zustand unterschiedlich? -> Uebernehmen
        // Der Taster war DEBOUNCE_MS lang stabil, also ist es ein echter Druck
        const bool timer_expired =
            (now_ms - _last_change[id - 1] >= DEBOUNCE_MS);
        const bool states_differ = (raw_now != deb_now);

        if (timer_expired && states_differ) {
            activeLow_setPressed(deb, id, raw_now);
            any_changed = true;
        }
    }

    // Rohzustand fuer naechsten Zyklus merken
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        _raw_prev[i] = raw[i];
    }

    return any_changed;
}
