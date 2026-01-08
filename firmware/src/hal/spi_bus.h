#ifndef SPI_BUS_H
#define SPI_BUS_H

#include <Arduino.h>
#include <SPI.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// =============================================================================
// SPI-Bus Abstraktion mit FreeRTOS Mutex
// =============================================================================
//
// Warum eine eigene Klasse statt direktem SPI-Zugriff?
// → Mutex schützt vor gleichzeitigem Zugriff (falls später mehrere Tasks)
// → SpiGuard (RAII) garantiert Transaction-Ende auch bei Exceptions
// → Zentrale Stelle für Bus-Initialisierung
//
// =============================================================================

class SpiBus {
public:
    // Initialisiert SPI-Hardware und erstellt Mutex
    void begin(int sck, int miso, int mosi);

    // Mutex-Zugriff (für SpiGuard)
    void lock();
    void unlock();

private:
    SemaphoreHandle_t mtx_ = nullptr;
};

// =============================================================================
// RAII Guard für SPI-Transaktionen
// =============================================================================
//
// Warum RAII statt manuellem begin/end?
// → Garantiert endTransaction() auch wenn Code früh returned
// → Garantiert unlock() auch bei Fehlern
// → Kompakter Code ohne vergessene Cleanup-Aufrufe
//
// Verwendung:
//   {
//       SpiGuard g(bus, settings);
//       SPI.transfer(...);
//   }  // ← Automatisch: endTransaction() + unlock()
//
// =============================================================================

class SpiGuard {
public:
    SpiGuard(SpiBus& bus, const SPISettings& settings)
        : bus_(bus) {
        bus_.lock();
        SPI.beginTransaction(settings);
    }

    ~SpiGuard() {
        SPI.endTransaction();
        bus_.unlock();
    }

    // Nicht kopierbar (würde doppeltes unlock verursachen)
    SpiGuard(const SpiGuard&) = delete;
    SpiGuard& operator=(const SpiGuard&) = delete;

private:
    SpiBus& bus_;
};

#endif // SPI_BUS_H
