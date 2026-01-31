#include "logic/debounce.h"

// =============================================================================
// Debouncer Implementation
// =============================================================================

void Debouncer::init() {
    // Alle Taster als "losgelassen" initialisieren (0xFF = Active-Low)
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        rawPrev_[i] = 0xFF;
    }

    // Timer auf 0 = sofortige Übernahme beim ersten echten Tastendruck
    for (size_t i = 0; i < BTN_COUNT; ++i) {
        lastChange_[i] = 0;
    }
}

bool Debouncer::update(uint32_t nowMs, const uint8_t* raw, uint8_t* deb) {
    bool anyChanged = false;

    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool rawNow = activeLow_pressed(raw, id);
        const bool rawPrev = activeLow_pressed(rawPrev_, id);
        const bool debNow = activeLow_pressed(deb, id);

        // Rohwert geändert? → Timer zurücksetzen
        // Das ist der "Prellen erkannt"-Moment
        if (rawNow != rawPrev) {
            lastChange_[id - 1] = nowMs;
        }

        // Timer abgelaufen UND Zustand unterschiedlich? → Übernehmen
        // Der Taster war DEBOUNCE_MS lang stabil, also ist es ein echter Druck
        const bool timerExpired = (nowMs - lastChange_[id - 1] >= DEBOUNCE_MS);
        const bool statesDiffer = (rawNow != debNow);

        if (timerExpired && statesDiffer) {
            activeLow_setPressed(deb, id, rawNow);
            anyChanged = true;
        }
    }

    // Rohzustand für nächsten Zyklus merken
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        rawPrev_[i] = raw[i];
    }

    return anyChanged;
}
