/**
 * @file config.h
 * @brief Hardware-Konfiguration für Selection Panel
 * @version 1.1.0
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// Hardware-Anzahl (HIER ÄNDERN FÜR SKALIERUNG)
// =============================================================================

constexpr size_t BTN_COUNT = 10;
constexpr size_t LED_COUNT = 10;

// Abgeleitet
constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8;
constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8;
constexpr size_t BTN_ICS = BTN_BYTES;
constexpr size_t LED_ICS = LED_BYTES;

// =============================================================================
// Pin-Belegung (XIAO ESP32-S3)
// =============================================================================

constexpr int PIN_SCK = D8;
constexpr int PIN_BTN_PS = D1;
constexpr int PIN_BTN_MISO = D9;
constexpr int PIN_LED_MOSI = D10;
constexpr int PIN_LED_RCK = D0;
constexpr int PIN_LED_OE = D2;

// =============================================================================
// PWM-Konfiguration
// =============================================================================

constexpr uint8_t PWM_DUTY = 50;
constexpr uint32_t PWM_FREQ = 1000;
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint8_t LEDC_RESOLUTION = 8;

// =============================================================================
// Timing
// =============================================================================

constexpr uint32_t LOOP_DELAY_MS = 5;
constexpr uint32_t CLOCK_DELAY_US = 2;
constexpr uint32_t DEBOUNCE_MS = 30;

#endif // CONFIG_H
