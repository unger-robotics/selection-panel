/**
 * @file cd4021.h
 * @brief CD4021B Treiber (Parallel-In/Serial-Out Schieberegister)
 *
 * Liest Taster ueber Hardware-SPI ein.
 *
 * Besonderheit "First-Bit-Problem":
 * Nach Parallel-Load liegt Q8 sofort an, BEVOR der erste Clock kommt.
 * SPI samplet aber erst NACH der ersten Flanke -> Bit 1 geht verloren.
 * Loesung: Erstes Bit vor SPI per digitalRead() sichern.
 */
#ifndef CD4021_H
#define CD4021_H

// =============================================================================
// INCLUDES
// =============================================================================

#include "config.h"
#include "hal/spi_bus.h"
#include <Arduino.h>

// =============================================================================
// CLASSES
// =============================================================================

/**
 * @brief Treiber fuer CD4021B Schieberegister (Taster-Input)
 */
class Cd4021 {
public:
    /**
     * @brief Initialisiert GPIO-Pins
     */
    void init();

    /**
     * @brief Liest alle Taster (mit First-Bit-Korrektur)
     * @param bus SPI-Bus Instanz
     * @param out Ausgabe-Array [BTN_BYTES]
     */
    void readRaw(SpiBus& bus, uint8_t* out);

private:
    SPISettings _spi{SPI_HZ_BTN, MSBFIRST, SPI_MODE_BTN};  /**< SPI-Einstellungen */
};

#endif // CD4021_H
