/**
 * @file bitops.h
 * @brief Bit-Adressierung fuer Taster und LEDs
 *
 * Abstrahiert die Hardware-Eigenheiten von CD4021B (MSB-first) und
 * 74HC595 (LSB-first). ID 1-10 ist menschenfreundlich, intern 0-basiert.
 */
#ifndef BITOPS_H
#define BITOPS_H

// =============================================================================
// INCLUDES
// =============================================================================

#include "config.h"
#include <Arduino.h>

// =============================================================================
// TASTER (CD4021B): MSB-first
// =============================================================================
// CD4021B liefert: Bit7 = P1 (Taster 1), Bit0 = P8 (Taster 8)
// Byte 0 enthaelt Taster 1-8, Byte 1 enthaelt Taster 9-16

/**
 * @brief Berechnet Byte-Index fuer Taster-ID
 * @param id Taster-ID (1-basiert)
 * @return Byte-Index im Array
 */
static inline uint8_t btn_byte(uint8_t id) { return (id - 1) / 8; }

/**
 * @brief Berechnet Bit-Position fuer Taster-ID (MSB-first)
 * @param id Taster-ID (1-basiert)
 * @return Bit-Position (7-0)
 */
static inline uint8_t btn_bit(uint8_t id) { return 7 - ((id - 1) % 8); }

// =============================================================================
// LEDs (74HC595): LSB-first
// =============================================================================
// 74HC595 sendet: Bit0 = QA (LED 1), Bit7 = QH (LED 8)
// Byte 0 enthaelt LED 1-8, Byte 1 enthaelt LED 9-16

/**
 * @brief Berechnet Byte-Index fuer LED-ID
 * @param id LED-ID (1-basiert)
 * @return Byte-Index im Array
 */
static inline uint8_t led_byte(uint8_t id) { return (id - 1) / 8; }

/**
 * @brief Berechnet Bit-Position fuer LED-ID (LSB-first)
 * @param id LED-ID (1-basiert)
 * @return Bit-Position (0-7)
 */
static inline uint8_t led_bit(uint8_t id) { return (id - 1) % 8; }

// =============================================================================
// ACTIVE-LOW HILFSFUNKTIONEN (fuer Taster)
// =============================================================================
// Taster sind Active-Low: 0 = gedrueckt, 1 = losgelassen

/**
 * @brief Prueft ob Taster gedrueckt ist (Active-Low)
 * @param arr Taster-Array
 * @param id Taster-ID (1-basiert)
 * @return true wenn gedrueckt
 */
static inline bool activeLow_pressed(const uint8_t *arr, uint8_t id) {
    const uint8_t byte_idx = btn_byte(id);
    const uint8_t bit_pos = btn_bit(id);
    return !(arr[byte_idx] & (1u << bit_pos));
}

/**
 * @brief Setzt Taster-Zustand (Active-Low)
 * @param arr Taster-Array
 * @param id Taster-ID (1-basiert)
 * @param pressed true = gedrueckt
 */
static inline void activeLow_setPressed(uint8_t *arr, uint8_t id,
                                        bool pressed) {
    const uint8_t byte_idx = btn_byte(id);
    const uint8_t bit_pos = btn_bit(id);
    if (pressed) {
        arr[byte_idx] &= ~(1u << bit_pos);
    } else {
        arr[byte_idx] |= (1u << bit_pos);
    }
}

// =============================================================================
// LED HILFSFUNKTIONEN
// =============================================================================

/**
 * @brief Prueft ob LED eingeschaltet ist
 * @param arr LED-Array
 * @param id LED-ID (1-basiert)
 * @return true wenn eingeschaltet
 */
static inline bool led_on(const uint8_t *arr, uint8_t id) {
    const uint8_t byte_idx = led_byte(id);
    const uint8_t bit_pos = led_bit(id);
    return (arr[byte_idx] >> bit_pos) & 1u;
}

/**
 * @brief Setzt LED-Zustand
 * @param arr LED-Array
 * @param id LED-ID (1-basiert)
 * @param on true = einschalten
 */
static inline void led_set(uint8_t *arr, uint8_t id, bool on) {
    const uint8_t byte_idx = led_byte(id);
    const uint8_t bit_pos = led_bit(id);
    if (on) {
        arr[byte_idx] |= (1u << bit_pos);
    } else {
        arr[byte_idx] &= ~(1u << bit_pos);
    }
}

#endif // BITOPS_H
