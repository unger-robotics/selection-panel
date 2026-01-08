// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>

// =============================================================================
// HARDWARE-KONFIGURATION: Selection Panel (Modulare Architektur)
// =============================================================================
//
// Schichtenmodell:
//   main.cpp          → Entry Point, Queue-Erstellung
//   app/              → FreeRTOS Tasks (io_task, serial_task)
//   logic/            → Geschäftslogik (debounce, selection)
//   drivers/          → Hardware-Treiber (cd4021, hc595)
//   hal/              → Hardware Abstraction (spi_bus)
//
// =============================================================================

// -----------------------------------------------------------------------------
// Anzahl der Ein-/Ausgänge
// -----------------------------------------------------------------------------
constexpr uint8_t BTN_COUNT = 10;
constexpr uint8_t LED_COUNT = 10;

// Bytes für Bit-Arrays (aufrunden: 10 Bits → 2 Bytes)
constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8;
constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8;

// -----------------------------------------------------------------------------
// Pin-Zuordnung (XIAO ESP32-S3)
// -----------------------------------------------------------------------------
// Gemeinsamer SPI-Bus für CD4021B und 74HC595
constexpr int PIN_SCK = D8;       // SPI-Takt
constexpr int PIN_BTN_PS = D1;    // CD4021B: Parallel/Serial Select
constexpr int PIN_BTN_MISO = D9;  // CD4021B: Daten (Q8)
constexpr int PIN_LED_MOSI = D10; // 74HC595: Daten (SER)
constexpr int PIN_LED_RCK = D0;   // 74HC595: Latch (STCP)
constexpr int PIN_LED_OE = D2;    // 74HC595: Output Enable (PWM, active-low)

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
// IO_PERIOD_MS: Abtastrate des IO-Tasks (5 ms = 200 Hz)
// Muss kürzer als DEBOUNCE_MS sein, damit Entprellung funktioniert.
constexpr uint32_t IO_PERIOD_MS = 5;

// DEBOUNCE_MS: Zeit, die ein Taster stabil sein muss (30 ms = sicher)
constexpr uint32_t DEBOUNCE_MS = 30;

// -----------------------------------------------------------------------------
// Selection-Verhalten
// -----------------------------------------------------------------------------
// true:  Auswahl bleibt nach Loslassen bestehen ("Latch")
// false: Auswahl erlischt wenn kein Taster gedrückt
constexpr bool LATCH_SELECTION = true;

// -----------------------------------------------------------------------------
// SPI-Einstellungen
// -----------------------------------------------------------------------------
// CD4021B: Schiebt bei steigender Flanke → Sample bei fallender (MODE1)
// 74HC595: Übernimmt bei steigender Flanke → Standard (MODE0)
constexpr uint32_t SPI_HZ_BTN = 500000UL;  // 500 kHz (Breadboard-sicher)
constexpr uint32_t SPI_HZ_LED = 1000000UL; // 1 MHz

constexpr uint8_t SPI_MODE_BTN = SPI_MODE1;
constexpr uint8_t SPI_MODE_LED = SPI_MODE0;

// -----------------------------------------------------------------------------
// PWM für LED-Helligkeit
// -----------------------------------------------------------------------------
// OE ist active-low: 0% Duty = volle Helligkeit, 100% = aus
// Code invertiert automatisch.
constexpr bool USE_OE_PWM = true;
constexpr uint8_t PWM_DUTY_PERCENT = 50;
constexpr uint32_t PWM_FREQ_HZ = 1000; // 1 kHz: flimmerfrei
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint8_t LEDC_RESOLUTION = 8; // 256 Stufen

// -----------------------------------------------------------------------------
// Serial-Protokoll (Phase 7: Raspberry Pi Bridge)
// -----------------------------------------------------------------------------
// Wichtig: Wenn SERIAL_PROTOCOL_ONLY=true, darf über USB nur das Protokoll
// ausgegeben werden (READY/FW/PRESS ...), sonst kann der Pi-Parser
// Phantom-Presses aus Debug-Zeilen/Zahlen erzeugen.
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr bool SERIAL_PROTOCOL_ONLY =
    true; // true: Pi-Protokoll, false: Debug-Logs
constexpr bool SERIAL_SEND_READY = true;   // "READY" beim Start
constexpr bool SERIAL_SEND_FW_LINE = true; // "FW ..." beim Start

// -----------------------------------------------------------------------------
// Debug-Logging
// -----------------------------------------------------------------------------
// LOG_VERBOSE_PER_ID: Detaillierte Ausgabe pro Taster/LED
// LOG_ON_RAW_CHANGE:  Auch unentprellte Änderungen loggen (zeigt Prellen)
constexpr bool LOG_VERBOSE_PER_ID = false;
constexpr bool LOG_ON_RAW_CHANGE = false;
constexpr uint8_t LOG_QUEUE_LEN =
    32; // größer: weniger Drop-Risiko bei Burst-Events

// -----------------------------------------------------------------------------
// FreeRTOS-Konfiguration
// -----------------------------------------------------------------------------
// Core 0: WiFi/BLE Stack (reserviert)
// Core 1: Anwendung (IO + Serial)
constexpr BaseType_t CORE_APP = 1;

// IO niedrig: Läuft periodisch mit vTaskDelayUntil
// Serial hoch: Soll Queue schnell leeren wenn CPU frei
constexpr UBaseType_t PRIO_IO = 5;
constexpr UBaseType_t PRIO_SERIAL = 2;

// -----------------------------------------------------------------------------
// LED-Update Policy
// -----------------------------------------------------------------------------
// Hintergrund: Beim CD4021-Read wird per SPI 0x00 getaktet -> der 74HC595
// schiebt dabei Nullen in sein Shiftregister (Ausgänge ändern sich erst beim
// Latch). Wenn RCK (Latch) sporadisch glitcht, können LEDs ausgehen. Refresh
// pro Zyklus stellt den korrekten Zustand spätestens nach IO_PERIOD_MS wieder
// her.
constexpr bool LED_REFRESH_EVERY_CYCLE = true;

static_assert(PWM_DUTY_PERCENT <= 100, "PWM_DUTY_PERCENT must be 0..100");
static_assert(BTN_COUNT > 0 && LED_COUNT > 0, "BTN/LED count must be > 0");

#endif // CONFIG_H
