#ifndef CD4021_H
#define CD4021_H

#include "config.h"
#include "hal/spi_bus.h"
#include <Arduino.h>

// =============================================================================
// CD4021B Treiber (Parallel-In/Serial-Out Schieberegister)
// =============================================================================
//
// Aufgabe: Liest 10 Taster über Hardware-SPI ein.
//
// Besonderheit "First-Bit-Problem":
// Nach Parallel-Load liegt Q8 sofort an, BEVOR der erste Clock kommt.
// SPI samplet aber erst NACH der ersten Flanke → Bit 1 geht verloren.
// Lösung: Erstes Bit vor SPI per digitalRead() sichern.
//
// =============================================================================

class Cd4021 {
public:
    // Initialisiert GPIO-Pins
    void init();

    // Liest alle Taster in out[BTN_BYTES] (mit First-Bit-Korrektur)
    void readRaw(SpiBus& bus, uint8_t* out);

private:
    // SPI-Einstellungen für CD4021B (MODE1, 500 kHz)
    SPISettings spi_{SPI_HZ_BTN, MSBFIRST, SPI_MODE_BTN};
};

#endif // CD4021_H
