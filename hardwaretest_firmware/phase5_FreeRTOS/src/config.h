#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// HARDWARE-KONFIGURATION: Selection Panel mit FreeRTOS
// =============================================================================
//
// Architektur: Zwei FreeRTOS-Tasks auf Core 1
//   - taskIO:     Hardware-Zugriff (SPI, GPIO) mit garantiertem Timing
//   - taskSerial: Debug-Ausgabe ohne Blockierung des IO-Tasks
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
// Gemeinsamer SPI-Bus für beide Schieberegister-Ketten
constexpr int PIN_SCK = D8;       // SPI-Takt
constexpr int PIN_BTN_PS = D1;    // CD4021B: Parallel/Serial Select
constexpr int PIN_BTN_MISO = D9;  // CD4021B: Daten (Q8)
constexpr int PIN_LED_MOSI = D10; // 74HC595: Daten (SER)
constexpr int PIN_LED_RCK = D0;   // 74HC595: Latch (STCP)
constexpr int PIN_LED_OE = D2;    // 74HC595: Output Enable (PWM, active-low)

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
// IO_PERIOD_MS bestimmt die Abtastrate: 5 ms → 200 Hz
// Schneller als DEBOUNCE_MS sein, damit Entprellung korrekt arbeitet.
constexpr uint32_t IO_PERIOD_MS = 5;

// Mechanische Taster prellen 5-20 ms. 30 ms gibt Sicherheitsmarge.
constexpr uint32_t DEBOUNCE_MS = 30;

// -----------------------------------------------------------------------------
// Selection-Verhalten
// -----------------------------------------------------------------------------
// LATCH_SELECTION = true:  Auswahl bleibt nach Loslassen bestehen
// LATCH_SELECTION = false: Auswahl erlischt wenn kein Taster gedrückt
constexpr bool LATCH_SELECTION = true;

// -----------------------------------------------------------------------------
// SPI-Einstellungen
// -----------------------------------------------------------------------------
// CD4021B: MODE1 (Sample bei fallender Flanke, IC schiebt bei steigender)
// 74HC595: MODE0 (Sample bei steigender Flanke, Standard)
constexpr uint32_t SPI_HZ_BTN = 500000UL;  // 500 kHz (Breadboard-sicher)
constexpr uint32_t SPI_HZ_LED = 1000000UL; // 1 MHz

constexpr uint8_t SPI_MODE_BTN = SPI_MODE1;
constexpr uint8_t SPI_MODE_LED = SPI_MODE0;

// -----------------------------------------------------------------------------
// PWM für LED-Helligkeit
// -----------------------------------------------------------------------------
// OE ist active-low: 0% Duty → volle Helligkeit, 100% → aus
// Code invertiert automatisch.
constexpr bool USE_OE_PWM = true;
constexpr uint8_t PWM_DUTY_PERCENT = 50; // Helligkeit in %
constexpr uint32_t PWM_FREQ_HZ = 1000;   // 1 kHz: flimmerfrei
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint8_t LEDC_RESOLUTION = 8; // 256 Stufen

// -----------------------------------------------------------------------------
// Debug-Logging
// -----------------------------------------------------------------------------
// LOG_VERBOSE_PER_ID: Zeigt jeden Taster/LED einzeln (hilfreich beim Debugging)
// LOG_ON_RAW_CHANGE:  Loggt auch unentprellte Änderungen (zeigt Prellen)
constexpr bool LOG_VERBOSE_PER_ID = true;
constexpr bool LOG_ON_RAW_CHANGE = false;
constexpr uint8_t LOG_QUEUE_LEN = 8; // Max. gepufferte Log-Events

// -----------------------------------------------------------------------------
// FreeRTOS-Konfiguration
// -----------------------------------------------------------------------------
// Core 0: WiFi/BLE Stack (reserviert)
// Core 1: Anwendung (IO + Serial)
constexpr BaseType_t CORE_APP = 1;

// Prioritäten: Höher = wichtiger
// IO niedrig: Läuft periodisch, braucht keine Vorrangbehandlung
// Serial hoch: Soll Queue schnell leeren wenn CPU frei
constexpr UBaseType_t PRIO_IO = 1;
constexpr UBaseType_t PRIO_SERIAL = 10;

#endif // CONFIG_H
