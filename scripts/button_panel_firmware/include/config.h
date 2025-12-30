/**
 * @file config.h
 * @brief Konfiguration für Button Panel (Prototyp / Produktion)
 * @version 2.2.0
 * @date 2025-12-26
 *
 * Hardware:
 * - Seeed XIAO ESP32-S3 (Dual-Core)
 * - 74HC595 (LED Output)
 * - CD4021BE (Button Input)
 *
 * Architektur:
 * - Core 0: Button-Scanning Task (hohe Prioritaet)
 * - Core 1: Serial-Kommunikation + LED-Steuerung (Arduino Loop)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// BUILD-MODUS: PROTOTYPE vs PRODUCTION
// =============================================================================

/**
 * PROTOTYPE_MODE aktivieren fuer 10x10 Testaufbau
 * Auskommentieren fuer 100x100 Produktionssystem
 */
#define PROTOTYPE_MODE

#ifdef PROTOTYPE_MODE
    constexpr uint8_t NUM_SHIFT_REGS = 2;    // 2x ICs = 16 Bits
    constexpr uint8_t NUM_LEDS       = 10;
    constexpr uint8_t NUM_BUTTONS    = 10;
#else
    constexpr uint8_t NUM_SHIFT_REGS = 13;   // 13x ICs = 104 Bits (100 genutzt)
    constexpr uint8_t NUM_LEDS       = 100;
    constexpr uint8_t NUM_BUTTONS    = 100;
#endif

// Abgeleitete Konstanten
constexpr uint8_t NUM_595_CHIPS      = NUM_SHIFT_REGS;
constexpr uint8_t NUM_4021_CHIPS     = NUM_SHIFT_REGS;
constexpr uint8_t TOTAL_OUTPUT_BITS  = NUM_595_CHIPS * 8;
constexpr uint8_t TOTAL_INPUT_BITS   = NUM_4021_CHIPS * 8;

// Maximale Anzahl (fuer statische Buffer)
constexpr uint8_t MAX_SHIFT_REGS     = 13;
constexpr uint8_t MAX_BUTTONS        = 100;

// =============================================================================
// PIN-DEFINITIONEN (ESP32-S3 XIAO)
// =============================================================================

// 74HC595 Output Chain (LEDs)
constexpr uint8_t PIN_595_DATA   = D0;   // GPIO1 - SER (Pin 14)
constexpr uint8_t PIN_595_CLOCK  = D1;   // GPIO2 - SRCLK (Pin 11)
constexpr uint8_t PIN_595_LATCH  = D2;   // GPIO3 - RCLK (Pin 12)

// CD4021BE Input Chain (Buttons)
constexpr uint8_t PIN_4021_DATA  = D3;   // GPIO4 - Q8 (Pin 3) - Input!
constexpr uint8_t PIN_4021_CLOCK = D4;   // GPIO5 - CLK (Pin 10)
constexpr uint8_t PIN_4021_LOAD  = D5;   // GPIO6 - P/S (Pin 9)

// =============================================================================
// TIMING-PARAMETER
// =============================================================================

// Schieberegister-Timing (Mikrosekunden)
// 74HC595: min 20ns Clock-Puls @ 5V
// CD4021:  min 180ns Clock-Puls @ 5V (langsamer!)
constexpr uint8_t SHIFT_DELAY_US = 1;    // 1us sicher fuer beide

// Button-Debouncing
constexpr uint32_t DEBOUNCE_TIME_MS = 50;

// Serial-Kommunikation
constexpr uint32_t SERIAL_BAUD = 115200;

// Polling-Intervall
constexpr uint32_t BUTTON_POLL_MS = 10;

// LED-Test Timing (non-blocking)
constexpr uint32_t LED_TEST_STEP_MS = 150;

// =============================================================================
// FREERTOS KONFIGURATION (DUAL-CORE)
// =============================================================================

// Task-Prioritaeten (hoeher = wichtiger)
constexpr UBaseType_t TASK_PRIORITY_BUTTONS = 2;
constexpr UBaseType_t TASK_PRIORITY_SERIAL  = 1;

// Stack-Groessen (in Words, nicht Bytes!)
constexpr uint32_t TASK_STACK_BUTTONS = 2048;
constexpr uint32_t TASK_STACK_SERIAL  = 4096;

// Core-Zuweisung
constexpr BaseType_t CORE_BUTTONS = 0;
constexpr BaseType_t CORE_MAIN    = 1;

// Queue-Groessen
constexpr uint8_t BUTTON_QUEUE_SIZE  = 16;
constexpr uint8_t COMMAND_QUEUE_SIZE = 8;

// =============================================================================
// CD4021 TIMING-HINWEIS
// =============================================================================
/**
 * WICHTIG: CD4021BE hat invertierte Load-Logik!
 *
 * CD4021:  P/S = HIGH -> Parallel Load (Daten einlesen)
 *          P/S = LOW  -> Serial Shift (Daten schieben)
 *
 * 74HC165: SH/LD = LOW  -> Parallel Load
 *          SH/LD = HIGH -> Serial Shift
 */

// =============================================================================
// BUTTON-ACTIVE-STATE
// =============================================================================
/**
 * Buttons mit Pull-up-Widerstaenden (10k) nach VCC.
 * Taster schliessen nach GND.
 *
 * Button offen:    HIGH (3.3V ueber Pull-up)
 * Button gedrueckt: LOW (GND ueber Taster)
 *
 * -> Active-LOW Logik
 */
constexpr bool BUTTON_ACTIVE_LOW = true;

// =============================================================================
// SERIAL-PROTOKOLL
// =============================================================================

constexpr uint8_t MAX_COMMAND_LENGTH = 32;

// =============================================================================
// WATCHDOG
// =============================================================================

constexpr uint32_t WATCHDOG_TIMEOUT_MS = 5000;

// =============================================================================
// DEBUG-OPTIONEN
// =============================================================================

// Debug-Ausgaben aktivieren (auskommentieren fuer Produktion)
// #define DEBUG_BUTTONS
// #define DEBUG_LEDS
// #define DEBUG_TIMING
// #define DEBUG_TASKS
// #define DEBUG_QUEUE_OVERFLOW

#endif // CONFIG_H
