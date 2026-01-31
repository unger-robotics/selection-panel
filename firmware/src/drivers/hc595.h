/**
 * @file hc595.h
 * @brief 74HC595 Treiber (Serial-In/Parallel-Out Schieberegister)
 *
 * Steuert LEDs ueber Hardware-SPI.
 *
 * Besonderheiten:
 * - Daisy-Chain: Letztes Byte zuerst senden (rutscht durch die Kette)
 * - Ghost-Maskierung: Unbenutzte Bits auf 0 setzen
 * - PWM ueber OE: Globale Helligkeitsregelung
 */
#ifndef HC595_H
#define HC595_H

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
 * @brief Treiber fuer 74HC595 Schieberegister (LED-Output)
 */
class Hc595 {
public:
    /**
     * @brief Initialisiert GPIO-Pins und PWM
     */
    void init();

    /**
     * @brief Setzt globale LED-Helligkeit
     * @param percent Helligkeit 0-100%
     */
    void setBrightness(uint8_t percent);

    /**
     * @brief Schreibt LED-Zustand ueber SPI und latcht
     * @param bus SPI-Bus Instanz
     * @param state LED-Zustand [LED_BYTES]
     */
    void write(SpiBus& bus, uint8_t* state);

private:
    /**
     * @brief Setzt unbenutzte Bits auf 0 (Ghost-LEDs verhindern)
     * @param state LED-Zustand Array
     */
    void maskUnused(uint8_t* state);

    /**
     * @brief Latch-Impuls: Uebernimmt Schieberegister -> Ausgaenge
     */
    void latch();

    SPISettings _spi{SPI_HZ_LED, MSBFIRST, SPI_MODE_LED};  /**< SPI-Einstellungen */
};

#endif // HC595_H
