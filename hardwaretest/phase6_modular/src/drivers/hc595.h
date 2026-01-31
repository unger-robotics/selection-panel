#ifndef HC595_H
#define HC595_H

#include "config.h"
#include "hal/spi_bus.h"
#include <Arduino.h>

// =============================================================================
// 74HC595 Treiber (Serial-In/Parallel-Out Schieberegister)
// =============================================================================
//
// Aufgabe: Steuert 10 LEDs über Hardware-SPI.
//
// Besonderheiten:
// - Daisy-Chain: Letztes Byte zuerst senden (rutscht durch die Kette)
// - Ghost-Maskierung: Unbenutzte Bits auf 0 setzen
// - PWM über OE: Globale Helligkeitsregelung
//
// =============================================================================

class Hc595 {
public:
    // Initialisiert GPIO-Pins und PWM
    void init();

    // Setzt globale LED-Helligkeit (0-100%)
    void setBrightness(uint8_t percent);

    // Schreibt LED-Zustand über SPI und latcht
    void write(SpiBus& bus, uint8_t* state);

private:
    // Setzt unbenutzte Bits auf 0 (Ghost-LEDs verhindern)
    void maskUnused(uint8_t* state);

    // Latch-Impuls: Übernimmt Schieberegister → Ausgänge
    void latch();

    // SPI-Einstellungen für 74HC595 (MODE0, 1 MHz)
    SPISettings spi_{SPI_HZ_LED, MSBFIRST, SPI_MODE_LED};
};

#endif // HC595_H
