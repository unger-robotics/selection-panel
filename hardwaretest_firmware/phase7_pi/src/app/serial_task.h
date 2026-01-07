// src/app/serial_task.h
#ifndef SERIAL_TASK_H
#define SERIAL_TASK_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <Arduino.h>

// =============================================================================
// Serial Task
// =============================================================================
//
// Verantwortung:
// - Empfängt Log-Events aus Queue → sendet PRESS/RELEASE an Pi
// - Empfängt Befehle vom Pi → steuert LEDs über Callback
// - Einzige Stelle die Serial I/O macht
//
// Protokoll (1-basiert, 3-stellig):
//   ESP32 → Pi:  READY, FW, PRESS 001, RELEASE 001, PONG, OK, ERROR
//   Pi → ESP32:  PING, STATUS, VERSION, HELP
//                LEDSET 001, LEDON 001, LEDOFF 001, LEDCLR, LEDALL
//
// =============================================================================

// LED-Befehle vom Pi
enum LedCommand : uint8_t {
    LED_CMD_SET,    // One-hot: nur diese LED an
    LED_CMD_ON,     // Additiv: LED einschalten
    LED_CMD_OFF,    // Additiv: LED ausschalten
    LED_CMD_CLEAR,  // Alle LEDs aus
    LED_CMD_ALL     // Alle LEDs an
};

// Callback-Typ für LED-Steuerung (implementiert in io_task)
typedef void (*LedControlCallback)(LedCommand cmd, uint8_t id);

// Startet den Serial-Task auf CORE_APP mit PRIO_SERIAL
void start_serial_task(QueueHandle_t logQueue);

// Registriert Callback für LED-Befehle vom Pi
void set_led_callback(LedControlCallback callback);

#endif // SERIAL_TASK_H
