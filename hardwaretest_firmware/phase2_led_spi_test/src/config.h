#pragma once
#include <Arduino.h>

// =============================================================================
// HARDWARE-KONFIGURATION: 10-Button Selection Panel (Prototyp)
// =============================================================================
//
// Aufbau: XIAO ESP32-S3 → 2× 74HC595 (LEDs) + 2× CD4021B (Taster, vorbereitet)
//         Accent Unit: 10 LEDs, skalierbar auf 100 (13× 74HC595)
//
// =============================================================================

// -----------------------------------------------------------------------------
// LED-Konfiguration
// -----------------------------------------------------------------------------
constexpr size_t LED_COUNT = 10;
constexpr size_t LED_BYTES =
    (LED_COUNT + 7) / 8; // Aufrunden: 10 LEDs → 2 Bytes
constexpr uint32_t CHASE_DELAY_MS = 300;

// Unbenutzte Bits im letzten Byte ausblenden:
// Bei 10 LEDs nutzen wir nur Bit 0-1 im zweiten Byte (Bits 2-7 = Ghost-LEDs)
constexpr uint8_t LED_LAST_MASK =
    (LED_COUNT % 8 == 0) ? 0xFF
                         : static_cast<uint8_t>((1u << (LED_COUNT % 8)) - 1u);

// -----------------------------------------------------------------------------
// Pin-Zuordnung (XIAO ESP32-S3)
// -----------------------------------------------------------------------------
// SPI-Bus (Hardware-SPI für stabiles Timing bei hohen Taktraten)
constexpr int PIN_SCK = D8;       // Gemeinsamer Takt für alle Schieberegister
constexpr int PIN_LED_MOSI = D10; // Daten zu 74HC595
constexpr int PIN_LED_RCK = D0;   // Latch: Übernimmt Schieberegister → Ausgänge

// Steuerung
constexpr int PIN_LED_OE = D2;   // Output Enable (active-low, PWM für Dimmen)
constexpr int PIN_BTN_PS = D1;   // CD4021B Parallel/Serial (HIGH = Shift-Mode)
constexpr int PIN_BTN_MISO = D9; // Daten von CD4021B (für spätere Nutzung)

// -----------------------------------------------------------------------------
// SPI-Einstellungen
// -----------------------------------------------------------------------------
// 1 MHz: Konservativer Wert für Breadboard-Aufbau mit Kabelkapazitäten.
// Produktiv auf PCB bis 10 MHz möglich.
constexpr uint32_t SPI_HZ = 1000000UL;

// -----------------------------------------------------------------------------
// PWM für Helligkeitsregelung (OE-Pin)
// -----------------------------------------------------------------------------
// OE ist active-low: 0% Duty → volle Helligkeit, 100% Duty → aus.
// Wir invertieren im Code, sodass PWM_DUTY = 80 auch 80% Helligkeit bedeutet.
constexpr bool USE_OE_PWM = true;
constexpr uint8_t PWM_DUTY = 80;    // Helligkeit in Prozent (0-100)
constexpr uint32_t PWM_FREQ = 1000; // 1 kHz: flimmerfrei, unhörbar
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint8_t LEDC_RESOLUTION = 8; // 256 Stufen
