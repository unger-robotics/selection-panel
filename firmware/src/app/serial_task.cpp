/**
 * @file serial_task.cpp
 * @brief Serial Task Implementation (v2.5.1 - Robuste USB-CDC Kommunikation)
 *
 * Problem: USB-CDC kann Nachrichten fragmentieren (z.B. "PRE" + "SS 001\n")
 * Loesung:
 *   1. Komplette Zeile als ein write() senden (nicht printf)
 *   2. Kurzer Delay nach flush() damit USB-Paket abgeschlossen wird
 *   3. Server hat zusaetzlich Fragment-Timeout
 */

// =============================================================================
// INCLUDES
// =============================================================================

#include "app/serial_task.h"

#include "bitops.h"
#include "config.h"
#include "types.h"
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// =============================================================================
// MODUL-LOKALE VARIABLEN
// =============================================================================

static QueueHandle_t _log_queue = nullptr;
static led_control_callback_t _led_callback = nullptr;

// Letzter aktiver Button (fuer RELEASE-Erkennung)
static uint8_t _last_active_id = 0;

// Serial-Eingabepuffer
static char _rx_buffer[64];
static size_t _rx_index = 0;

// TX-Puffer fuer atomische Sends
static char _tx_buffer[64];

// =============================================================================
// PRIVATE DEBUG HILFSFUNKTIONEN
// =============================================================================

/**
 * @brief Gibt Byte als Binaerzahl aus
 */
static void print_binary(uint8_t value) {
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 1u);
    }
}

/**
 * @brief Gibt Byte-Array mit Label aus
 */
static void print_byte_array(const char *label, const uint8_t *arr,
                             size_t bytes) {
    Serial.print(label);
    for (size_t i = 0; i < bytes; ++i) {
        if (i) {
            Serial.print(" | ");
        }
        Serial.printf("IC%d: ", (int)i);
        print_binary(arr[i]);
        Serial.printf(" (0x%02X)", arr[i]);
    }
    Serial.println();
}

/**
 * @brief Gibt Liste der gedrueckten Taster aus
 */
static void print_pressed_list(const uint8_t *deb) {
    Serial.print("Pressed: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (activeLow_pressed(deb, id)) {
            Serial.printf("%u ", id);
            any = true;
        }
    }
    if (!any) {
        Serial.print('-');
    }
    Serial.println();
}

/**
 * @brief Gibt detaillierte Taster-Info aus
 */
static void print_buttons_verbose(const uint8_t *raw, const uint8_t *deb) {
    Serial.println("Buttons per ID (RAW/DEB)  [pressed=1 | released=0]");
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        Serial.printf("  T%02u  IC%u b%u   RAW=%u  DEB=%u\n", id,
                      (unsigned)btn_byte(id), (unsigned)btn_bit(id),
                      activeLow_pressed(raw, id) ? 1u : 0u,
                      activeLow_pressed(deb, id) ? 1u : 0u);
    }
}

/**
 * @brief Gibt detaillierte LED-Info aus
 */
static void print_leds_verbose(const uint8_t *led) {
    Serial.println("LEDs per ID (STATE)  [on=1 | off=0]");
    for (uint8_t id = 1; id <= LED_COUNT; ++id) {
        Serial.printf("  LED%02u  IC%u b%u   STATE=%u\n", id,
                      (unsigned)led_byte(id), (unsigned)led_bit(id),
                      led_on(led, id) ? 1u : 0u);
    }
}

// =============================================================================
// PRIVATE SERIAL AUSGABE
// =============================================================================

/**
 * @brief Sendet eine Zeile atomar ueber USB-CDC
 * @note USB-CDC hat 64-Byte Pakete, printf kann fragmentieren
 */
static void send_line(const char *line) {
    Serial.print(line);
    Serial.print('\n');
    Serial.flush();
    delayMicroseconds(2000); // 2ms USB-CDC Paket abschliessen lassen
}

/**
 * @brief Formatiert und sendet eine Zeile atomar
 */
static void send_linef(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(_tx_buffer, sizeof(_tx_buffer), fmt, args);
    va_end(args);
    send_line(_tx_buffer);
}

// =============================================================================
// PRIVATE PROTOKOLL-FUNKTIONEN (ESP32 -> Pi)
// =============================================================================

static void send_pong() { send_line("PONG"); }

static void send_ok() { send_line("OK"); }

static void send_error(const char *msg) { send_linef("ERROR %s", msg); }

static void send_version() { send_line("FW selection-panel v2.5.1"); }

static void send_help() {
    send_line("Commands: PING, STATUS, VERSION, HELP");
    send_line("          LEDSET n, LEDON n, LEDOFF n, LEDCLR, LEDALL");
}

static void send_status() {
    send_linef("CURLED %u", _last_active_id);
    send_linef("BTNS %u", BTN_COUNT);
    send_linef("LEDS %u", LED_COUNT);
    send_linef("HEAP %u", ESP.getFreeHeap());
    send_linef("MODE %s", BTN_COUNT <= 10 ? "PROTOTYPE" : "PRODUCTION");
    send_ok();
}

static void send_press(uint8_t id) { send_linef("PRESS %03u", id); }

static void send_release(uint8_t id) { send_linef("RELEASE %03u", id); }

// =============================================================================
// PRIVATE BEFEHLSVERARBEITUNG (Pi -> ESP32)
// =============================================================================

/**
 * @brief Parst eine ID (001-100) aus einem String
 * @return ID (1-LED_COUNT) oder -1 bei Fehler
 */
static int parse_id(const char *str) {
    while (*str == ' ') {
        str++; // Leerzeichen ueberspringen
    }
    int id = atoi(str);
    if (id >= 1 && id <= LED_COUNT) {
        return id;
    }
    return -1;
}

/**
 * @brief Verarbeitet einen empfangenen Befehl
 */
static void process_command(const char *cmd) {
    if (cmd[0] == '\0') {
        return; // Leerzeilen ignorieren
    }

    // --- Einfache Befehle ---
    if (strcmp(cmd, "PING") == 0) {
        send_pong();
        return;
    }

    if (strcmp(cmd, "VERSION") == 0) {
        send_version();
        return;
    }

    if (strcmp(cmd, "HELP") == 0) {
        send_help();
        return;
    }

    if (strcmp(cmd, "STATUS") == 0) {
        send_status();
        return;
    }

    // --- LED-Befehle ohne ID ---
    if (strcmp(cmd, "LEDCLR") == 0) {
        if (_led_callback != nullptr) {
            _led_callback(LED_CMD_CLEAR, 0);
            _last_active_id = 0;
        }
        send_ok();
        return;
    }

    if (strcmp(cmd, "LEDALL") == 0) {
        if (_led_callback != nullptr) {
            _led_callback(LED_CMD_ALL, 0);
        }
        send_ok();
        return;
    }

    // --- LED-Befehle mit ID ---
    if (strncmp(cmd, "LEDSET ", 7) == 0) {
        int id = parse_id(cmd + 7);
        if (id > 0) {
            if (_led_callback != nullptr) {
                _led_callback(LED_CMD_SET, (uint8_t)id);
                _last_active_id = (uint8_t)id;
            }
            send_ok();
        } else {
            send_error("INVALID_ID");
        }
        return;
    }

    if (strncmp(cmd, "LEDON ", 6) == 0) {
        int id = parse_id(cmd + 6);
        if (id > 0) {
            if (_led_callback != nullptr) {
                _led_callback(LED_CMD_ON, (uint8_t)id);
            }
            send_ok();
        } else {
            send_error("INVALID_ID");
        }
        return;
    }

    if (strncmp(cmd, "LEDOFF ", 7) == 0) {
        int id = parse_id(cmd + 7);
        if (id > 0) {
            if (_led_callback != nullptr) {
                _led_callback(LED_CMD_OFF, (uint8_t)id);
            }
            send_ok();
        } else {
            send_error("INVALID_ID");
        }
        return;
    }

    // Unbekannter Befehl
    send_error("UNKNOWN_CMD");
}

/**
 * @brief Liest Serial-Eingabe (non-blocking)
 */
static void read_serial_input() {
    while (Serial.available()) {
        char c = Serial.read();

        // Zeilenende erkannt
        if (c == '\n' || c == '\r') {
            if (_rx_index > 0) {
                _rx_buffer[_rx_index] = '\0';
                process_command(_rx_buffer);
                _rx_index = 0;
            }
        }
        // Zeichen zum Puffer hinzufuegen
        else if (_rx_index < sizeof(_rx_buffer) - 1) {
            _rx_buffer[_rx_index++] = c;
        }
        // Puffer voll -> verwerfen und neu starten
        else {
            _rx_index = 0;
        }
    }
}

// =============================================================================
// TASK-FUNKTION
// =============================================================================

/**
 * @brief Hauptschleife des Serial-Tasks
 */
static void serial_task_function(void *) {
    Serial.begin(SERIAL_BAUD);
    delay(100); // USB-CDC stabilisieren

    if (SERIAL_PROTOCOL_ONLY) {
        // Protokoll-Modus: Nur READY und FW senden
        if (SERIAL_SEND_READY) {
            send_line("READY");
        }
        if (SERIAL_SEND_FW_LINE) {
            send_line("FW selection-panel v2.5.1");
        }
    } else {
        // Debug-Modus: Ausfuehrlicher Header
        Serial.println();
        Serial.println("========================================");
        Serial.println("Selection Panel v2.5.1");
        Serial.println("========================================");
        Serial.printf("BTN_COUNT:       %u\n", BTN_COUNT);
        Serial.printf("LED_COUNT:       %u\n", LED_COUNT);
        Serial.printf("IO_PERIOD_MS:    %u\n", IO_PERIOD_MS);
        Serial.printf("DEBOUNCE_MS:     %u\n", DEBOUNCE_MS);
        Serial.printf("LATCH_SELECTION: %s\n",
                      LATCH_SELECTION ? "true" : "false");
        Serial.println("========================================");
        send_line("READY");
    }

    log_event_t event = {};

    for (;;) {
        // 1) Serial-Eingabe pruefen (Befehle vom Pi)
        read_serial_input();

        // 2) Queue mit Timeout lesen
        if (xQueueReceive(_log_queue, &event, pdMS_TO_TICKS(10)) == pdTRUE) {

            if (SERIAL_PROTOCOL_ONLY) {
                // --- Protokoll-Modus: Nur PRESS/RELEASE senden ---
                if (event.active_changed) {
                    if (event.active_id > 0 && event.active_id <= BTN_COUNT) {
                        // Neuer Button aktiv -> PRESS senden
                        send_press(event.active_id);
                        _last_active_id = event.active_id;
                    } else if (_last_active_id > 0) {
                        // Kein Button mehr aktiv -> RELEASE senden
                        send_release(_last_active_id);
                        _last_active_id = 0;
                    }
                }
                continue;
            }

            // --- Debug-Modus: Ausfuehrliche Ausgabe ---
            if (event.active_changed) {
                if (event.active_id > 0) {
                    Serial.printf(">>> PRESS %03u\n", event.active_id);
                    _last_active_id = event.active_id;
                } else if (_last_active_id > 0) {
                    Serial.printf(">>> RELEASE %03u\n", _last_active_id);
                    _last_active_id = 0;
                }
            }

            Serial.println("---");
            print_byte_array("BTN RAW:    ", event.raw, BTN_BYTES);
            print_byte_array("BTN DEB:    ", event.deb, BTN_BYTES);
            Serial.printf("Active LED (One-Hot): %u\n", event.active_id);
            print_byte_array("LED STATE:  ", event.led, LED_BYTES);
            print_pressed_list(event.deb);

            if (LOG_VERBOSE_PER_ID) {
                print_buttons_verbose(event.raw, event.deb);
                print_leds_verbose(event.led);
            }

            vTaskDelay(1);
        }
    }
}

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

void start_serial_task(QueueHandle_t log_queue) {
    _log_queue = log_queue;
    xTaskCreatePinnedToCore(serial_task_function, "Serial", 8192, nullptr,
                            PRIO_SERIAL, nullptr, CORE_APP);
}

void set_led_callback(led_control_callback_t callback) {
    _led_callback = callback;
}
