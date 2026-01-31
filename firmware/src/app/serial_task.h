/**
 * @file serial_task.h
 * @brief Serial Task fuer Pi-Kommunikation
 *
 * Verantwortung:
 * - Empfaengt Log-Events aus Queue -> sendet PRESS/RELEASE an Pi
 * - Empfaengt Befehle vom Pi -> steuert LEDs ueber Callback
 * - Einzige Stelle die Serial I/O macht
 *
 * Protokoll (1-basiert, 3-stellig):
 *   ESP32 -> Pi:  READY, FW, PRESS 001, RELEASE 001, PONG, OK, ERROR
 *   Pi -> ESP32:  PING, STATUS, VERSION, HELP
 *                 LEDSET 001, LEDON 001, LEDOFF 001, LEDCLR, LEDALL
 */
#ifndef SERIAL_TASK_H
#define SERIAL_TASK_H

// =============================================================================
// INCLUDES
// =============================================================================

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <Arduino.h>

// =============================================================================
// TYPES
// =============================================================================

/**
 * @brief LED-Befehle vom Pi
 */
typedef enum led_command {
    LED_CMD_SET,    /**< One-hot: nur diese LED an */
    LED_CMD_ON,     /**< Additiv: LED einschalten */
    LED_CMD_OFF,    /**< Additiv: LED ausschalten */
    LED_CMD_CLEAR,  /**< Alle LEDs aus */
    LED_CMD_ALL     /**< Alle LEDs an */
} led_command_e;

/**
 * @brief Callback-Typ fuer LED-Steuerung (implementiert in io_task)
 */
typedef void (*led_control_callback_t)(led_command_e cmd, uint8_t id);

// =============================================================================
// OEFFENTLICHE FUNKTIONEN
// =============================================================================

/**
 * @brief Startet den Serial-Task auf CORE_APP mit PRIO_SERIAL
 * @param log_queue Queue fuer Log-Events
 */
void start_serial_task(QueueHandle_t log_queue);

/**
 * @brief Registriert Callback fuer LED-Befehle vom Pi
 * @param callback Callback-Funktion
 */
void set_led_callback(led_control_callback_t callback);

#endif // SERIAL_TASK_H
