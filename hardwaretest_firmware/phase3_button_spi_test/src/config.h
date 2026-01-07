#pragma once
#include <Arduino.h>

// =============================================================================
// HARDWARE-KONFIGURATION: CD4021B Taster-Test (ohne LEDs)
// =============================================================================
//
// Aufbau: XIAO ESP32-S3 → 2× CD4021B in Daisy-Chain
//         Accent Unit Prototyp: 10 Taster, skalierbar auf 100
//
// =============================================================================

// -----------------------------------------------------------------------------
// Taster-Konfiguration
// -----------------------------------------------------------------------------
constexpr size_t BTN_COUNT = 10;
constexpr size_t BTN_BYTES =
    (BTN_COUNT + 7) / 8; // Aufrunden: 10 Taster → 2 Bytes

// -----------------------------------------------------------------------------
// Pin-Zuordnung (XIAO ESP32-S3)
// -----------------------------------------------------------------------------
// SPI-Bus (shared zwischen LED- und Button-Kette)
constexpr int PIN_SCK = D8;       // SPI-Takt
constexpr int PIN_SPI_MOSI = D10; // Daten zu 74HC595
constexpr int PIN_BTN_MISO = D9;  // Daten von CD4021B (Q8)

// Button-Steuerung
constexpr int PIN_BTN_PS = D1; // Parallel/Serial: HIGH=Load, LOW=Shift

// LED-Steuerung (für definierten Ausgangszustand)
constexpr int PIN_LED_RCK = D0; // Latch: Übernimmt Schieberegister → Ausgänge
constexpr int PIN_LED_OE = D2;  // Output Enable (active-low)

// -----------------------------------------------------------------------------
// SPI-Einstellungen
// -----------------------------------------------------------------------------
// CD4021B schiebt Daten bei steigender Clock-Flanke hinaus.
// SPI_MODE1 (CPOL=0, CPHA=1): Sample bei fallender Flanke → stabile Daten.
// 500 kHz: Konservativ für Breadboard, CD4021B verträgt bis 3 MHz.
constexpr uint32_t SPI_HZ = 500000UL;
constexpr uint8_t SPI_MODE_BTN = SPI_MODE1;

// -----------------------------------------------------------------------------
// Debounce-Parameter
// -----------------------------------------------------------------------------
// Mechanische Taster prellen typisch 5-20 ms. 30 ms gibt Sicherheitsmarge
// ohne spürbare Verzögerung für den Benutzer.
constexpr bool USE_DEBOUNCE = true;
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint32_t LOOP_DELAY_MS = 2; // ~500 Hz Abtastrate
