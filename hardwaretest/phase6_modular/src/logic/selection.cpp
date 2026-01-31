#include "logic/selection.h"

// =============================================================================
// Selection Implementation
// =============================================================================

void Selection::init() {
    // Alle Taster als "losgelassen" initialisieren
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        debPrev_[i] = 0xFF;
    }
}

// Hilfsfunktion: Ist irgendein Taster gedrückt?
static bool anyPressed(const uint8_t* deb) {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (deb[i] != 0xFF) return true;  // Active-Low: != 0xFF = gedrückt
    }
    return false;
}

bool Selection::update(const uint8_t* debNow, uint8_t& activeId) {
    const uint8_t prevActive = activeId;
    uint8_t newActive = activeId;

    // Flanken-Erkennung: Wer wurde gerade gedrückt?
    // Iteriert von ID 1 bis BTN_COUNT, "last press wins" bei mehreren Flanken
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool nowPressed = activeLow_pressed(debNow, id);
        const bool prevPressed = activeLow_pressed(debPrev_, id);

        // Steigende Flanke = gerade gedrückt (war los, ist jetzt gedrückt)
        if (nowPressed && !prevPressed) {
            newActive = id;
        }
    }

    // LATCH_SELECTION = false: Auswahl erlischt wenn nichts mehr gedrückt
    if (!LATCH_SELECTION && !anyPressed(debNow)) {
        newActive = 0;
    }

    // Zustand für nächsten Zyklus merken
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        debPrev_[i] = debNow[i];
    }

    activeId = newActive;
    return (activeId != prevActive);
}
