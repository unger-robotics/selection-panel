/**
 * @file spi_bus.cpp
 * @brief SPI-Bus Implementation
 */

// =============================================================================
// INCLUDES
// =============================================================================

#include "hal/spi_bus.h"

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

void SpiBus::begin(int sck, int miso, int mosi) {
    // Mutex nur einmal erstellen (idempotent bei mehrfachem Aufruf)
    if (_mtx == nullptr) {
        _mtx = xSemaphoreCreateMutex();
    }

    // ESP32 SPI.begin() erlaubt flexible Pin-Zuordnung
    // -1 fuer SS = kein Hardware-SS (wir nutzen Software-CS)
    SPI.begin(sck, miso, mosi, -1);
}

void SpiBus::lock() {
    if (_mtx != nullptr) {
        // Blockiert bis Mutex frei (portMAX_DELAY = unendlich)
        xSemaphoreTake(_mtx, portMAX_DELAY);
    }
}

void SpiBus::unlock() {
    if (_mtx != nullptr) {
        xSemaphoreGive(_mtx);
    }
}
