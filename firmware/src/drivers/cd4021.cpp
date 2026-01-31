/**
 * @file cd4021.cpp
 * @brief CD4021B Implementation
 */

// =============================================================================
// INCLUDES
// =============================================================================

#include "drivers/cd4021.h"

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

void Cd4021::init() {
    // P/S Pin: Steuert Parallel-Load vs. Serial-Shift
    pinMode(PIN_BTN_PS, OUTPUT);
    digitalWrite(PIN_BTN_PS, LOW); // Start im Shift-Mode

    // MISO: Dateneingang (Q8 des CD4021B)
    // INPUT_PULLUP als Fallback falls CD4021B nicht bestueckt
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
}

void Cd4021::readRaw(SpiBus &bus, uint8_t *out) {
    // -------------------------------------------------------------------------
    // Schritt 1: Parallel Load
    // -------------------------------------------------------------------------
    // P/S = HIGH: Taster-Zustaende werden ins Register "fotografiert"
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(5); // Hold-Zeit fuer stabiles Einlesen

    // -------------------------------------------------------------------------
    // Schritt 2: Umschalten auf Shift-Mode
    // -------------------------------------------------------------------------
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(1); // Stabilisierungszeit

    // -------------------------------------------------------------------------
    // Schritt 3: KRITISCH - Erstes Bit vor SPI sichern
    // -------------------------------------------------------------------------
    // Q8 liegt jetzt bereits am Ausgang an (noch vor dem ersten Clock!)
    // Wenn wir das nicht separat lesen, verlieren wir Taster 1.
    const uint8_t first_bit = digitalRead(PIN_BTN_MISO) ? 1u : 0u;

    // -------------------------------------------------------------------------
    // Schritt 4: Restliche Bits per SPI einlesen
    // -------------------------------------------------------------------------
    uint8_t rx[BTN_BYTES] = {0};

    {
        SpiGuard guard(bus, _spi);
        for (size_t i = 0; i < BTN_BYTES; ++i) {
            rx[i] = SPI.transfer(0x00);
        }
    }

    // -------------------------------------------------------------------------
    // Schritt 5: Bit-Korrektur
    // -------------------------------------------------------------------------
    // rx[] enthaelt Bits 2-17 (verschoben), wir brauchen Bits 1-16.
    // Loesung: Alles um 1 nach rechts, first_bit als MSB von Byte 0 einsetzen.
    //
    // out[0]: [first_bit | rx[0] bits 7-1]
    // out[1]: [rx[0] bit 0 | rx[1] bits 7-1]

    out[0] = static_cast<uint8_t>((first_bit << 7) | (rx[0] >> 1));

    for (size_t i = 1; i < BTN_BYTES; ++i) {
        out[i] = static_cast<uint8_t>(((rx[i - 1] & 0x01) << 7) | (rx[i] >> 1));
    }
}
