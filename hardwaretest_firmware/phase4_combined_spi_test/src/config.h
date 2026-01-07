#pragma once
#include <Arduino.h>

// =============================================================================
// HARDWARE-KONFIGURATION: Selection Panel – Button→LED (1:1 + One-Hot)
// =============================================================================
//
// Aufbau: XIAO ESP32-S3 → 2× CD4021B (Taster) + 2× 74HC595 (LEDs)
//         Beide Ketten am gemeinsamen SPI-Bus, unterschiedliche SPI-Modi.
//
// =============================================================================

// -----------------------------------------------------------------------------
// Anzahl der Ein-/Ausgänge
// -----------------------------------------------------------------------------
constexpr size_t BTN_COUNT = 10;
constexpr size_t LED_COUNT = 10;

constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8; // 10 → 2 Bytes
constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8; // 10 → 2 Bytes

// Unbenutzte Bits im letzten LED-Byte ausblenden (Bits 2-7 = Ghost-LEDs)
constexpr uint8_t LED_LAST_MASK =
    (LED_COUNT % 8 == 0) ? 0xFF
                         : static_cast<uint8_t>((1u << (LED_COUNT % 8)) - 1u);

// -----------------------------------------------------------------------------
// Pin-Zuordnung (XIAO ESP32-S3)
// -----------------------------------------------------------------------------
// Gemeinsamer SPI-Bus
constexpr int PIN_SCK = D8; // Takt für beide Schieberegister-Ketten

// CD4021B (Taster-Eingabe)
constexpr int PIN_BTN_PS = D1;   // Parallel/Serial: HIGH=Load, LOW=Shift
constexpr int PIN_BTN_MISO = D9; // Daten von CD4021B (Q8)

// 74HC595 (LED-Ausgabe)
constexpr int PIN_LED_MOSI = D10; // Daten zu 74HC595 (SER)
constexpr int PIN_LED_RCK = D0;   // Latch: Übernimmt Schieberegister → Ausgänge
constexpr int PIN_LED_OE = D2;    // Output Enable (active-low, PWM für Dimmen)

// -----------------------------------------------------------------------------
// SPI-Einstellungen (unterschiedliche Modi für CD4021B und 74HC595)
// -----------------------------------------------------------------------------
// CD4021B: Schiebt Daten bei steigender Flanke hinaus.
//          SPI_MODE1 (CPOL=0, CPHA=1) samplet bei fallender Flanke → stabil.
constexpr uint32_t SPI_4021_HZ =
    500000UL; // 500 kHz (konservativ für Breadboard)
constexpr uint8_t SPI_4021_MODE = SPI_MODE1;

// 74HC595: Übernimmt Daten bei steigender Flanke.
//          SPI_MODE0 (CPOL=0, CPHA=0) ist Standard für diesen IC.
constexpr uint32_t SPI_595_HZ = 1000000UL; // 1 MHz
constexpr uint8_t SPI_595_MODE = SPI_MODE0;

// -----------------------------------------------------------------------------
// Debounce-Parameter
// -----------------------------------------------------------------------------
// Mechanische Taster prellen 5-20 ms. 30 ms gibt Sicherheitsmarge.
constexpr bool USE_DEBOUNCE = true;
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr uint32_t LOOP_DELAY_MS = 2; // ~500 Hz Abtastrate

// -----------------------------------------------------------------------------
// PWM für Helligkeitsregelung
// -----------------------------------------------------------------------------
// OE ist active-low: Duty wird im Code invertiert.
constexpr bool USE_OE_PWM = true;
constexpr uint8_t PWM_DUTY = 60;    // Helligkeit in Prozent
constexpr uint32_t PWM_FREQ = 1000; // 1 kHz: flimmerfrei
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint8_t LEDC_RESOLUTION = 8; // 256 Stufen
