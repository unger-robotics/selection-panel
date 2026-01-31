#include "hal/spi_bus.h"

// =============================================================================
// SPI-Bus Implementation
// =============================================================================

void SpiBus::begin(int sck, int miso, int mosi) {
    // Mutex nur einmal erstellen (idempotent bei mehrfachem Aufruf)
    if (!mtx_) {
        mtx_ = xSemaphoreCreateMutex();
    }

    // ESP32 SPI.begin() erlaubt flexible Pin-Zuordnung
    // -1 für SS = kein Hardware-SS (wir nutzen Software-CS)
    SPI.begin(sck, miso, mosi, -1);
}

void SpiBus::lock() {
    if (mtx_) {
        // Blockiert bis Mutex frei (portMAX_DELAY = unendlich)
        xSemaphoreTake(mtx_, portMAX_DELAY);
    }
}

void SpiBus::unlock() {
    if (mtx_) {
        xSemaphoreGive(mtx_);
    }
}
