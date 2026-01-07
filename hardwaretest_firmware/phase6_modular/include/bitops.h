#ifndef BITOPS_H
#define BITOPS_H

#include "config.h"
#include <Arduino.h>

// =============================================================================
// Bit-Adressierung für Taster und LEDs
// =============================================================================
//
// Warum separate Funktionen statt direkter Zugriff?
// → CD4021B und 74HC595 haben unterschiedliche Bit-Reihenfolgen
// → Abstrahiert die Hardware-Eigenheiten an einer Stelle
// → ID 1-10 ist menschenfreundlich, intern 0-basiert
//
// =============================================================================

// -----------------------------------------------------------------------------
// Taster (CD4021B): MSB-first
// -----------------------------------------------------------------------------
// CD4021B liefert: Bit7 = P1 (Taster 1), Bit0 = P8 (Taster 8)
// Byte 0 enthält Taster 1-8, Byte 1 enthält Taster 9-16

static inline uint8_t btn_byte(uint8_t id) {
    return (id - 1) / 8;
}

static inline uint8_t btn_bit(uint8_t id) {
    return 7 - ((id - 1) % 8);  // MSB-first: ID 1 → Bit 7
}

// -----------------------------------------------------------------------------
// LEDs (74HC595): LSB-first
// -----------------------------------------------------------------------------
// 74HC595 sendet: Bit0 = QA (LED 1), Bit7 = QH (LED 8)
// Byte 0 enthält LED 1-8, Byte 1 enthält LED 9-16

static inline uint8_t led_byte(uint8_t id) {
    return (id - 1) / 8;
}

static inline uint8_t led_bit(uint8_t id) {
    return (id - 1) % 8;  // LSB-first: ID 1 → Bit 0
}

// -----------------------------------------------------------------------------
// Active-Low Hilfsfunktionen (für Taster)
// -----------------------------------------------------------------------------
// Taster sind Active-Low: 0 = gedrückt, 1 = losgelassen
// Diese Funktionen abstrahieren die Invertierung.

static inline bool activeLow_pressed(const uint8_t* arr, uint8_t id) {
    const uint8_t byte = btn_byte(id);
    const uint8_t bit = btn_bit(id);
    return !(arr[byte] & (1u << bit));  // 0 = gedrückt → true
}

static inline void activeLow_setPressed(uint8_t* arr, uint8_t id, bool pressed) {
    const uint8_t byte = btn_byte(id);
    const uint8_t bit = btn_bit(id);
    if (pressed) {
        arr[byte] &= ~(1u << bit);  // Pressed → Bit auf 0
    } else {
        arr[byte] |= (1u << bit);   // Released → Bit auf 1
    }
}

#endif // BITOPS_H
