/**
 * @file io_task.cpp
 * @brief IO Task Implementation
 */

// =============================================================================
// INCLUDES
// =============================================================================

#include "app/io_task.h"

#include "bitops.h"
#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "app/serial_task.h"
#include "drivers/cd4021.h"
#include "drivers/hc595.h"
#include "hal/spi_bus.h"
#include "logic/debounce.h"
#include "logic/selection.h"

// =============================================================================
// TYPES
// =============================================================================

/**
 * @brief LED-Befehl Struktur (fuer Queue)
 */
typedef struct led_cmd_event {
    led_command_e cmd; /**< Befehlstyp */
    uint8_t id;        /**< LED-ID (1-basiert) */
} led_cmd_event_t;

// =============================================================================
// MODUL-LOKALE VARIABLEN
// =============================================================================

static QueueHandle_t _log_queue = nullptr;
static QueueHandle_t _led_cmd_queue = nullptr;

// Hardware-Abstraktionen
static SpiBus _spi_bus;
static Cd4021 _buttons;
static Hc595 _leds;

// Logik-Module
static Debouncer _debouncer;
static Selection _selection;

// Zustaende
static uint8_t _btn_raw[BTN_BYTES];
static uint8_t _btn_raw_prev[BTN_BYTES];
static uint8_t _btn_debounced[BTN_BYTES];
static uint8_t _led_state[LED_BYTES];
static uint8_t _active_id = 0;

// Remote-Modus: Wenn true, steuert der Pi die LEDs
static bool _remote_mode = false;

// =============================================================================
// PRIVATE HILFSFUNKTIONEN
// =============================================================================

/**
 * @brief Vergleicht zwei Arrays
 * @return true wenn identisch
 */
static inline bool arrays_equal(const uint8_t *a, const uint8_t *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Prueft ob irgendein Taster gedrueckt
 */
static inline bool any_pressed(const uint8_t *deb) {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (deb[i] != 0xFF) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Setzt LED-Zustand basierend auf ID (One-Hot: nur eine LED an)
 * @param id LED-ID (0 = alle aus, 1-LED_COUNT = diese LED an)
 */
static void build_one_hot_led(uint8_t id) {
    memset(_led_state, 0x00, LED_BYTES);
    if (id >= 1 && id <= LED_COUNT) {
        const uint8_t byte_idx = led_byte(id);
        const uint8_t bit_pos = led_bit(id);
        _led_state[byte_idx] |= (1u << bit_pos);
    }
}

/**
 * @brief Setzt alle LEDs an
 */
static void set_all_leds() {
    memset(_led_state, 0xFF, LED_BYTES);
    // Ungenutzte Bits maskieren
    if (LED_COUNT % 8 != 0) {
        const uint8_t used_bits = LED_COUNT % 8;
        const uint8_t mask = static_cast<uint8_t>((1u << used_bits) - 1);
        _led_state[LED_BYTES - 1] &= mask;
    }
}

/**
 * @brief Schaltet einzelne LED ein (additiv)
 */
static void set_led_on(uint8_t id) {
    if (id >= 1 && id <= LED_COUNT) {
        const uint8_t byte_idx = led_byte(id);
        const uint8_t bit_pos = led_bit(id);
        _led_state[byte_idx] |= (1u << bit_pos);
    }
}

/**
 * @brief Schaltet einzelne LED aus (additiv)
 */
static void set_led_off(uint8_t id) {
    if (id >= 1 && id <= LED_COUNT) {
        const uint8_t byte_idx = led_byte(id);
        const uint8_t bit_pos = led_bit(id);
        _led_state[byte_idx] &= ~(1u << bit_pos);
    }
}

/**
 * @brief LED-Callback (wird vom Serial-Task aufgerufen)
 */
static void led_control_callback(led_command_e cmd, uint8_t id) {
    if (_led_cmd_queue == nullptr) {
        return;
    }

    led_cmd_event_t event = {cmd, id};
    // Non-blocking: Wenn Queue voll, wird Befehl verworfen
    xQueueSend(_led_cmd_queue, &event, 0);
}

/**
 * @brief LED-Befehle verarbeiten (aus Queue)
 * @return true wenn LED-Zustand geaendert
 */
static bool process_led_commands() {
    led_cmd_event_t event;
    bool led_changed = false;

    // Alle wartenden Befehle verarbeiten
    while (xQueueReceive(_led_cmd_queue, &event, 0) == pdTRUE) {
        switch (event.cmd) {
        case LED_CMD_SET:
            build_one_hot_led(event.id);
            _active_id = event.id;
            _remote_mode = true;
            led_changed = true;
            break;

        case LED_CMD_ON:
            set_led_on(event.id);
            _remote_mode = true;
            led_changed = true;
            break;

        case LED_CMD_OFF:
            set_led_off(event.id);
            _remote_mode = true;
            led_changed = true;
            break;

        case LED_CMD_CLEAR:
            memset(_led_state, 0x00, LED_BYTES);
            _active_id = 0;
            _remote_mode = false; // Lokale Kontrolle wieder aktiv
            led_changed = true;
            break;

        case LED_CMD_ALL:
            set_all_leds();
            _remote_mode = true;
            led_changed = true;
            break;
        }
    }

    return led_changed;
}

// =============================================================================
// TASK-FUNKTION
// =============================================================================

/**
 * @brief Hauptschleife des IO-Tasks
 */
static void io_task_function(void *) {
    // -------------------------------------------------------------------------
    // Initialisierung (einmalig)
    // -------------------------------------------------------------------------
    _spi_bus.begin(PIN_SCK, PIN_BTN_MISO, PIN_LED_MOSI);

    _buttons.init();
    _leds.init();

    _debouncer.init();
    _selection.init();

    // LED-Callback registrieren
    set_led_callback(led_control_callback);

    // Alle Taster als "losgelassen" initialisieren
    memset(_btn_raw, 0xFF, BTN_BYTES);
    memset(_btn_raw_prev, 0xFF, BTN_BYTES);
    memset(_btn_debounced, 0xFF, BTN_BYTES);
    memset(_led_state, 0x00, LED_BYTES);

    _active_id = 0;
    build_one_hot_led(_active_id);
    _leds.write(_spi_bus, _led_state);

    // Fuer praezises Timing: Startzeit merken
    TickType_t last_wake = xTaskGetTickCount();

    // -------------------------------------------------------------------------
    // Hauptschleife (endlos)
    // -------------------------------------------------------------------------
    for (;;) {
        // Warten bis naechste Periode (kompensiert Ausfuehrungszeit)
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(IO_PERIOD_MS));

        // ---------------------------------------------------------------------
        // 0. LED-Befehle vom Pi verarbeiten
        // ---------------------------------------------------------------------
        bool led_cmd_processed = process_led_commands();

        // ---------------------------------------------------------------------
        // 1. Taster einlesen
        // ---------------------------------------------------------------------
        _buttons.readRaw(_spi_bus, _btn_raw);
        const bool raw_changed =
            !arrays_equal(_btn_raw, _btn_raw_prev, BTN_BYTES);

        // ---------------------------------------------------------------------
        // 2. Entprellen
        // ---------------------------------------------------------------------
        const uint32_t now = millis();
        const bool deb_changed =
            _debouncer.update(now, _btn_raw, _btn_debounced);

        // ---------------------------------------------------------------------
        // 3. Auswahl aktualisieren
        // ---------------------------------------------------------------------
        const bool active_changed =
            _selection.update(_btn_debounced, _active_id);

        // ---------------------------------------------------------------------
        // 4. LEDs aktualisieren
        // ---------------------------------------------------------------------
        // Lokaler Tastendruck beendet Remote-Modus und uebernimmt Kontrolle
        if (active_changed && _active_id > 0) {
            _remote_mode = false;
            build_one_hot_led(_active_id);
        }
        // Ohne Remote-Modus: LEDs folgen lokaler Auswahl
        else if (active_changed && !_remote_mode) {
            build_one_hot_led(_active_id);
        }

        // LED-Hardware aktualisieren
        if (LED_REFRESH_EVERY_CYCLE || led_cmd_processed || active_changed) {
            _leds.write(_spi_bus, _led_state);
        }

        // ---------------------------------------------------------------------
        // 5. Log-Event erstellen und senden
        // ---------------------------------------------------------------------
        const bool should_log =
            deb_changed || active_changed || (LOG_ON_RAW_CHANGE && raw_changed);

        const bool not_empty = any_pressed(_btn_debounced) || active_changed;

        if (should_log && not_empty && _log_queue != nullptr) {
            log_event_t event = {};
            event.ms = now;
            memcpy(event.raw, _btn_raw, BTN_BYTES);
            memcpy(event.deb, _btn_debounced, BTN_BYTES);
            memcpy(event.led, _led_state, LED_BYTES);
            event.active_id = _active_id;
            event.raw_changed = raw_changed;
            event.deb_changed = deb_changed;
            event.active_changed = active_changed;

            // Nicht-blockierend: Bei voller Queue wird Event verworfen
            xQueueSend(_log_queue, &event, 0);
        }

        // Raw-Zustand fuer naechsten Zyklus merken
        memcpy(_btn_raw_prev, _btn_raw, BTN_BYTES);
    }
}

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

void start_io_task(QueueHandle_t log_queue) {
    _log_queue = log_queue;

    // LED-Befehls-Queue erstellen
    _led_cmd_queue = xQueueCreate(8, sizeof(led_cmd_event_t));

    xTaskCreatePinnedToCore(io_task_function, "IO", 8192, nullptr, PRIO_IO,
                            nullptr, CORE_APP);
}
