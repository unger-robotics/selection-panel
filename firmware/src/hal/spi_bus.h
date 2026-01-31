/**
 * @file spi_bus.h
 * @brief SPI-Bus Abstraktion mit FreeRTOS Mutex
 *
 * Stellt thread-sichere SPI-Transaktionen bereit.
 * SpiGuard (RAII) garantiert Transaction-Ende auch bei Exceptions.
 */
#ifndef SPI_BUS_H
#define SPI_BUS_H

// =============================================================================
// INCLUDES
// =============================================================================

#include <Arduino.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// =============================================================================
// CLASSES
// =============================================================================

/**
 * @brief SPI-Bus Wrapper mit Mutex-Schutz
 */
class SpiBus {
public:
    /**
     * @brief Initialisiert SPI-Hardware und erstellt Mutex
     * @param sck Clock-Pin
     * @param miso MISO-Pin
     * @param mosi MOSI-Pin
     */
    void begin(int sck, int miso, int mosi);

    /**
     * @brief Sperrt den Bus (blockiert bis frei)
     */
    void lock();

    /**
     * @brief Gibt den Bus frei
     */
    void unlock();

private:
    SemaphoreHandle_t _mtx = nullptr;  /**< FreeRTOS Mutex */
};

/**
 * @brief RAII Guard fuer SPI-Transaktionen
 *
 * Verwendung:
 * @code
 * {
 *     SpiGuard g(bus, settings);
 *     SPI.transfer(...);
 * }  // Automatisch: endTransaction() + unlock()
 * @endcode
 */
class SpiGuard {
public:
    /**
     * @brief Startet SPI-Transaktion mit Lock
     * @param bus SpiBus-Instanz
     * @param settings SPI-Einstellungen
     */
    SpiGuard(SpiBus& bus, const SPISettings& settings)
        : _bus(bus) {
        _bus.lock();
        SPI.beginTransaction(settings);
    }

    /**
     * @brief Beendet Transaktion und gibt Lock frei
     */
    ~SpiGuard() {
        SPI.endTransaction();
        _bus.unlock();
    }

    // Nicht kopierbar (wuerde doppeltes unlock verursachen)
    SpiGuard(const SpiGuard&) = delete;
    SpiGuard& operator=(const SpiGuard&) = delete;

private:
    SpiBus& _bus;  /**< Referenz auf den Bus */
};

#endif // SPI_BUS_H
