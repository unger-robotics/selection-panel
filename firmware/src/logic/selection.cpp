/**
 * @file selection.cpp
 * @brief Selection Implementation
 */

// =============================================================================
// INCLUDES
// =============================================================================

#include "logic/selection.h"

// =============================================================================
// PRIVATE HILFSFUNKTIONEN
// =============================================================================

/**
 * @brief Prueft ob irgendein Taster gedrueckt ist
 * @param deb Entprellter Zustand
 * @return true wenn mindestens ein Taster gedrueckt
 */
static bool any_pressed(const uint8_t *deb) {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (deb[i] != 0xFF) {
            return true; // Active-Low: != 0xFF = gedrueckt
        }
    }
    return false;
}

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

void Selection::init() {
    // Alle Taster als "losgelassen" initialisieren
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        _deb_prev[i] = 0xFF;
    }
}

bool Selection::update(const uint8_t *deb_now, uint8_t &active_id) {
    const uint8_t prev_active = active_id;
    uint8_t new_active = active_id;

    // Flanken-Erkennung: Wer wurde gerade gedrueckt?
    // Iteriert von ID 1 bis BTN_COUNT, "last press wins" bei mehreren Flanken
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool now_pressed = activeLow_pressed(deb_now, id);
        const bool prev_pressed = activeLow_pressed(_deb_prev, id);

        // Steigende Flanke = gerade gedrueckt (war los, ist jetzt gedrueckt)
        if (now_pressed && !prev_pressed) {
            new_active = id;
        }
    }

    // LATCH_SELECTION = false: Auswahl erlischt wenn nichts mehr gedrueckt
    if (!LATCH_SELECTION && !any_pressed(deb_now)) {
        new_active = 0;
    }

    // Zustand fuer naechsten Zyklus merken
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        _deb_prev[i] = deb_now[i];
    }

    active_id = new_active;
    return (active_id != prev_active);
}
