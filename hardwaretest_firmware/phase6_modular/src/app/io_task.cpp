#include "app/io_task.h"

#include "bitops.h"
#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "drivers/cd4021.h"
#include "drivers/hc595.h"
#include "hal/spi_bus.h"
#include "logic/debounce.h"
#include "logic/selection.h"

// =============================================================================
// IO Task Implementation
// =============================================================================

// Modul-lokale Variablen (nur in diesem File sichtbar)
static QueueHandle_t logQueue_ = nullptr;

// Hardware-Abstraktionen
static SpiBus spiBus_;
static Cd4021 buttons_;
static Hc595 leds_;

// Logik-Module
static Debouncer debouncer_;
static Selection selection_;

// Zustände
static uint8_t btnRaw_[BTN_BYTES];
static uint8_t btnRawPrev_[BTN_BYTES];
static uint8_t btnDebounced_[BTN_BYTES];
static uint8_t ledState_[LED_BYTES];
static uint8_t activeId_ = 0;

// -----------------------------------------------------------------------------
// Hilfsfunktionen
// -----------------------------------------------------------------------------

static inline bool arraysEqual(const uint8_t *a, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

static inline bool anyPressed(const uint8_t *deb) {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (deb[i] != 0xFF)
            return true;
    }
    return false;
}

// Setzt LED-Zustand basierend auf activeId (One-Hot: nur eine LED an)
static void buildOneHotLED(uint8_t activeId) {
    memset(ledState_, 0x00, LED_BYTES);

    if (activeId >= 1 && activeId <= LED_COUNT) {
        const uint8_t byte = led_byte(activeId);
        const uint8_t bit = led_bit(activeId);
        ledState_[byte] |= (1u << bit);
    }
}

// -----------------------------------------------------------------------------
// Task-Funktion
// -----------------------------------------------------------------------------

static void ioTaskFunction(void *) {
    // =========================================================================
    // Initialisierung (einmalig)
    // =========================================================================
    spiBus_.begin(PIN_SCK, PIN_BTN_MISO, PIN_LED_MOSI);

    buttons_.init();
    leds_.init();

    debouncer_.init();
    selection_.init();

    // Alle Taster als "losgelassen" initialisieren
    memset(btnRaw_, 0xFF, BTN_BYTES);
    memset(btnRawPrev_, 0xFF, BTN_BYTES);
    memset(btnDebounced_, 0xFF, BTN_BYTES);
    memset(ledState_, 0x00, LED_BYTES);

    activeId_ = 0;
    buildOneHotLED(activeId_);
    leds_.write(spiBus_, ledState_);

    // Für präzises Timing: Startzeit merken
    TickType_t lastWake = xTaskGetTickCount();

    // =========================================================================
    // Hauptschleife (endlos)
    // =========================================================================
    for (;;) {
        // Warten bis nächste Periode (kompensiert Ausführungszeit)
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(IO_PERIOD_MS));

        // ---------------------------------------------------------------------
        // 1. Taster einlesen
        // ---------------------------------------------------------------------
        buttons_.readRaw(spiBus_, btnRaw_);
        const bool rawChanged = !arraysEqual(btnRaw_, btnRawPrev_, BTN_BYTES);

        // ---------------------------------------------------------------------
        // 2. Entprellen
        // ---------------------------------------------------------------------
        const uint32_t now = millis();
        const bool debChanged = debouncer_.update(now, btnRaw_, btnDebounced_);

        // ---------------------------------------------------------------------
        // 3. Auswahl aktualisieren
        // ---------------------------------------------------------------------
        const bool activeChanged = selection_.update(btnDebounced_, activeId_);

        // ---------------------------------------------------------------------
        // 4. LEDs aktualisieren
        // ---------------------------------------------------------------------
        if (activeChanged) {
            buildOneHotLED(activeId_);
        }

        if (LED_REFRESH_EVERY_CYCLE) {
            // Refresh immer (wichtig bei gemeinsamem SCK: Button-Read schiebt
            // 0x00 in den 595)
            leds_.write(spiBus_, ledState_);
        } else {
            // Alter Modus: nur bei Änderung schreiben
            if (activeChanged) {
                leds_.write(spiBus_, ledState_);
            }
        }

        // ---------------------------------------------------------------------
        // 5. Log-Event erstellen und senden
        // ---------------------------------------------------------------------
        const bool shouldLog =
            debChanged || activeChanged || (LOG_ON_RAW_CHANGE && rawChanged);

        // Keine "leeren" Blöcke: Nur loggen wenn Taster gedrückt oder Änderung
        const bool notEmpty = anyPressed(btnDebounced_) || activeChanged;

        if (shouldLog && notEmpty && logQueue_) {
            LogEvent event = {};
            event.ms = now;
            memcpy(event.raw, btnRaw_, BTN_BYTES);
            memcpy(event.deb, btnDebounced_, BTN_BYTES);
            memcpy(event.led, ledState_, LED_BYTES);
            event.activeId = activeId_;
            event.rawChanged = rawChanged;
            event.debChanged = debChanged;
            event.activeChanged = activeChanged;

            // Nicht-blockierend: Bei voller Queue wird Event verworfen
            // IO-Timing bleibt stabil, Logging ist nicht kritisch
            xQueueSend(logQueue_, &event, 0);
        }

        // Raw-Zustand für nächsten Zyklus merken
        memcpy(btnRawPrev_, btnRaw_, BTN_BYTES);
    }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void start_io_task(QueueHandle_t logQueue) {
    logQueue_ = logQueue;

    xTaskCreatePinnedToCore(ioTaskFunction, // Task-Funktion
                            "IO",           // Name (für Debugging)
                            8192,           // Stack-Größe in Bytes
                            nullptr,        // Parameter (nicht verwendet)
                            PRIO_IO,        // Priorität
                            nullptr,        // Task-Handle (nicht benötigt)
                            CORE_APP        // Core (1 = App, 0 = WiFi/BLE)
    );
}
