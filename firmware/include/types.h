/**
 * @file types.h
 * @brief Gemeinsame Datentypen fuer das Selection Panel
 *
 * Definiert strukturierte Typen fuer die Kommunikation zwischen Tasks.
 */
#ifndef TYPES_H
#define TYPES_H

// =============================================================================
// INCLUDES
// =============================================================================

#include "config.h"
#include <Arduino.h>

// =============================================================================
// TYPES
// =============================================================================

/**
 * @brief Snapshot des Systemzustands fuer die Queue zwischen IO und Serial.
 *
 * Ein Queue-Element = ein atomarer Snapshot.
 * Keine Race-Conditions zwischen Feldern.
 */
typedef struct log_event {
    uint32_t ms;            /**< Zeitstempel (fuer Debugging) */
    uint8_t raw[BTN_BYTES]; /**< Rohzustand der Taster */
    uint8_t deb[BTN_BYTES]; /**< Entprellter Zustand */
    uint8_t led[LED_BYTES]; /**< LED-Ausgabezustand */
    uint8_t active_id;      /**< Aktive Auswahl (0 = keine, 1-10 = ID) */
    bool raw_changed;       /**< Flag: Raw hat sich geaendert */
    bool deb_changed;       /**< Flag: Debounced hat sich geaendert */
    bool active_changed;    /**< Flag: Auswahl hat sich geaendert */
} log_event_t;

#endif // TYPES_H
