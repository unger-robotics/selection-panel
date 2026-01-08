// src/app/io_task.cpp
#include "app/io_task.h"

#include "bitops.h"
#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "app/serial_task.h"  // Für LedCommand enum und set_led_callback
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
static QueueHandle_t ledCmdQueue_ = nullptr;  // Queue für LED-Befehle vom Pi

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

// Remote-Modus: Wenn true, steuert der Pi die LEDs
static bool remoteMode_ = false;

// =============================================================================
// LED-Befehl Struktur (für Queue)
// =============================================================================

struct LedCmdEvent {
    LedCommand cmd;
    uint8_t id;
};

// -----------------------------------------------------------------------------
// Hilfsfunktionen
// -----------------------------------------------------------------------------

static inline bool arraysEqual(const uint8_t *a, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

static inline bool anyPressed(const uint8_t *deb) {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (deb[i] != 0xFF) return true;
    }
    return false;
}

// Setzt LED-Zustand basierend auf ID (One-Hot: nur eine LED an)
static void buildOneHotLED(uint8_t id) {
    memset(ledState_, 0x00, LED_BYTES);
    if (id >= 1 && id <= LED_COUNT) {
        const uint8_t byte = led_byte(id);
        const uint8_t bit = led_bit(id);
        ledState_[byte] |= (1u << bit);
    }
}

// Setzt alle LEDs an
static void setAllLEDs() {
    memset(ledState_, 0xFF, LED_BYTES);
    // Ungenutzte Bits maskieren
    if (LED_COUNT % 8 != 0) {
        const uint8_t usedBits = LED_COUNT % 8;
        const uint8_t mask = static_cast<uint8_t>((1u << usedBits) - 1);
        ledState_[LED_BYTES - 1] &= mask;
    }
}

// Schaltet einzelne LED ein (additiv)
static void setLedOn(uint8_t id) {
    if (id >= 1 && id <= LED_COUNT) {
        const uint8_t byte = led_byte(id);
        const uint8_t bit = led_bit(id);
        ledState_[byte] |= (1u << bit);
    }
}

// Schaltet einzelne LED aus (additiv)
static void setLedOff(uint8_t id) {
    if (id >= 1 && id <= LED_COUNT) {
        const uint8_t byte = led_byte(id);
        const uint8_t bit = led_bit(id);
        ledState_[byte] &= ~(1u << bit);
    }
}

// -----------------------------------------------------------------------------
// LED-Callback (wird vom Serial-Task aufgerufen)
// -----------------------------------------------------------------------------

static void ledControlCallback(LedCommand cmd, uint8_t id) {
    if (ledCmdQueue_ == nullptr) return;
    
    LedCmdEvent event = {cmd, id};
    // Non-blocking: Wenn Queue voll, wird Befehl verworfen
    xQueueSend(ledCmdQueue_, &event, 0);
}

// -----------------------------------------------------------------------------
// LED-Befehle verarbeiten (aus Queue)
// -----------------------------------------------------------------------------

static bool processLedCommands() {
    LedCmdEvent event;
    bool ledChanged = false;
    
    // Alle wartenden Befehle verarbeiten
    while (xQueueReceive(ledCmdQueue_, &event, 0) == pdTRUE) {
        switch (event.cmd) {
            case LED_CMD_SET:
                buildOneHotLED(event.id);
                activeId_ = event.id;
                remoteMode_ = true;
                ledChanged = true;
                break;
                
            case LED_CMD_ON:
                setLedOn(event.id);
                remoteMode_ = true;
                ledChanged = true;
                break;
                
            case LED_CMD_OFF:
                setLedOff(event.id);
                remoteMode_ = true;
                ledChanged = true;
                break;
                
            case LED_CMD_CLEAR:
                memset(ledState_, 0x00, LED_BYTES);
                activeId_ = 0;
                remoteMode_ = false;  // Lokale Kontrolle wieder aktiv
                ledChanged = true;
                break;
                
            case LED_CMD_ALL:
                setAllLEDs();
                remoteMode_ = true;
                ledChanged = true;
                break;
        }
    }
    
    return ledChanged;
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

    // LED-Callback registrieren
    set_led_callback(ledControlCallback);

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
        // 0. LED-Befehle vom Pi verarbeiten
        // ---------------------------------------------------------------------
        bool ledCmdProcessed = processLedCommands();

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
        // Lokaler Tastendruck beendet Remote-Modus und übernimmt Kontrolle
        if (activeChanged && activeId_ > 0) {
            remoteMode_ = false;
            buildOneHotLED(activeId_);
        }
        // Ohne Remote-Modus: LEDs folgen lokaler Auswahl
        else if (activeChanged && !remoteMode_) {
            buildOneHotLED(activeId_);
        }

        // LED-Hardware aktualisieren
        if (LED_REFRESH_EVERY_CYCLE || ledCmdProcessed || activeChanged) {
            leds_.write(spiBus_, ledState_);
        }

        // ---------------------------------------------------------------------
        // 5. Log-Event erstellen und senden
        // ---------------------------------------------------------------------
        const bool shouldLog =
            debChanged || activeChanged || (LOG_ON_RAW_CHANGE && rawChanged);

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
    
    // LED-Befehls-Queue erstellen
    ledCmdQueue_ = xQueueCreate(8, sizeof(LedCmdEvent));

    xTaskCreatePinnedToCore(
        ioTaskFunction,
        "IO",
        8192,
        nullptr,
        PRIO_IO,
        nullptr,
        CORE_APP
    );
}
