/**
 * @file config.h
 * @brief Hardware-Konfiguration für Selection Panel
 * @version 1.1.0
 * 
 * Zentrale Konfiguration für:
 * - Pin-Belegung (XIAO ESP32-S3)
 * - Anzahl Taster/LEDs
 * - PWM-Einstellungen
 * - Timing-Konstanten
 * 
 * SKALIERUNG:
 * Aktuell:  10 Taster/LEDs (2x CD4021B, 2x 74HC595)
 * Ziel:    100 Taster/LEDs (13x CD4021B, 13x 74HC595)
 * 
 * Für 100: Nur BTN_COUNT und LED_COUNT ändern!
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// Hardware-Anzahl (HIER ÄNDERN FÜR SKALIERUNG)
// =============================================================================

constexpr size_t BTN_COUNT = 10;   // Anzahl Taster (max. 104 bei 13 ICs)
constexpr size_t LED_COUNT = 10;   // Anzahl LEDs (max. 104 bei 13 ICs)

// Abgeleitet (automatisch berechnet)
constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8;  // 10→2, 100→13
constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8;  // 10→2, 100→13
constexpr size_t BTN_ICS = BTN_BYTES;              // Anzahl CD4021B
constexpr size_t LED_ICS = LED_BYTES;              // Anzahl 74HC595

// =============================================================================
// Pin-Belegung (XIAO ESP32-S3)
// =============================================================================

// Shared SPI Bus (bleibt gleich bei Skalierung)
constexpr int PIN_SCK = D8;        // Clock (alle ICs)

// CD4021B (Taster-Eingabe)
constexpr int PIN_BTN_PS = D1;     // Parallel/Serial Load
constexpr int PIN_BTN_MISO = D9;   // Daten vom IC (Q8)

// 74HC595 (LED-Ausgabe)
constexpr int PIN_LED_MOSI = D10;  // Daten zum IC (SI)
constexpr int PIN_LED_RCK = D0;    // Latch (RCLK)
constexpr int PIN_LED_OE = D2;     // Output Enable (PWM)

// =============================================================================
// PWM-Konfiguration
// =============================================================================

constexpr uint8_t PWM_DUTY = 50;          // Standard-Helligkeit (%)
constexpr uint32_t PWM_FREQ = 1000;       // PWM-Frequenz (Hz)
constexpr uint8_t LEDC_CHANNEL = 0;       // ESP32 LEDC Kanal
constexpr uint8_t LEDC_RESOLUTION = 8;    // 8-Bit (0-255)

// =============================================================================
// Timing
// =============================================================================

constexpr uint32_t LOOP_DELAY_MS = 10;    // Hauptschleife (ms)
constexpr uint32_t CLOCK_DELAY_US = 2;    // SPI Clock-Puls (µs)

// =============================================================================
// Bit-Zuordnung (Hardware-bedingt, MSB/LSB)
// =============================================================================

// CD4021B (Taster):
//   - Shift: MSB first (Bit 7 kommt zuerst raus)
//   - Q8 (P8) → Bit 7 → Taster 1 im IC
//   - Verdrahtung: P8=T1, P7=T2, ..., P1=T8
//
// 74HC595 (LED):
//   - Shift: MSB first (Bit 7 geht zuerst rein)
//   - QA → Bit 0 → LED 1 im IC (LSB-Zuordnung)
//   - Verdrahtung: QA=L1, QB=L2, ..., QH=L8
//
// Zusammenfassung:
//   BTN: Bit 7 = ID 1 (MSB-basiert)
//   LED: Bit 0 = ID 1 (LSB-basiert)
//
// Die API abstrahiert das: isPressed(5) und setLED(5, true) funktionieren!

// =============================================================================
// Compile-Time Checks
// =============================================================================

static_assert(BTN_COUNT <= 104, "Max. 104 Taster (13 ICs)");
static_assert(LED_COUNT <= 104, "Max. 104 LEDs (13 ICs)");
static_assert(BTN_COUNT > 0, "Mindestens 1 Taster");
static_assert(LED_COUNT > 0, "Mindestens 1 LED");

#endif // CONFIG_H
