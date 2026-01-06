/**
 * @file main.cpp
 * @brief Phase 5 Simple: Zeitbasiertes Debouncing
 * @version 1.0.0
 * 
 * Einfaches Debouncing: Zustand wird erst übernommen,
 * wenn er für DEBOUNCE_MS (30ms) stabil war.
 */

#include <Arduino.h>
#include "config.h"

// =============================================================================
// Zustand
// =============================================================================

static uint8_t btnRaw[BTN_BYTES];
static uint8_t btnRawLast[BTN_BYTES];
static uint8_t btnDebounced[BTN_BYTES];
static uint32_t btnLastChange[BTN_COUNT];
static uint8_t ledState[LED_BYTES];

// =============================================================================
// Low-Level
// =============================================================================

static inline void clockPulse() {
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(CLOCK_DELAY_US);
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(CLOCK_DELAY_US);
}

static void readButtons() {
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(2);
    
    for (size_t ic = 0; ic < BTN_BYTES; ++ic) {
        uint8_t byte = 0;
        for (int8_t bit = 7; bit >= 0; --bit) {
            if (digitalRead(PIN_BTN_MISO)) {
                byte |= (1 << bit);
            }
            clockPulse();
        }
        btnRaw[ic] = byte;
    }
}

static void writeLEDs() {
    for (int16_t ic = LED_BYTES - 1; ic >= 0; --ic) {
        uint8_t byte = ledState[ic];
        for (int8_t bit = 7; bit >= 0; --bit) {
            digitalWrite(PIN_LED_MOSI, (byte >> bit) & 1);
            clockPulse();
        }
    }
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_LED_RCK, LOW);
}

static void setLEDBrightness(uint8_t percent) {
    uint8_t duty = 255 - (percent * 255 / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

// =============================================================================
// Debouncing
// =============================================================================

static bool getRawBit(uint8_t id) {
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = 7 - ((id - 1) % 8);
    return !(btnRaw[ic] & (1 << bit));
}

static bool getDebouncedBit(uint8_t id) {
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = 7 - ((id - 1) % 8);
    return !(btnDebounced[ic] & (1 << bit));
}

static void setDebouncedBit(uint8_t id, bool pressed) {
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = 7 - ((id - 1) % 8);
    if (pressed) {
        btnDebounced[ic] &= ~(1 << bit);
    } else {
        btnDebounced[ic] |= (1 << bit);
    }
}

static void debounceButtons() {
    uint32_t now = millis();
    
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        bool rawPressed = getRawBit(id);
        bool debouncedPressed = getDebouncedBit(id);
        
        uint8_t ic = (id - 1) / 8;
        uint8_t bit = 7 - ((id - 1) % 8);
        bool rawLast = !(btnRawLast[ic] & (1 << bit));
        
        if (rawPressed != rawLast) {
            btnLastChange[id - 1] = now;
        }
        
        if ((now - btnLastChange[id - 1] >= DEBOUNCE_MS) && 
            (rawPressed != debouncedPressed)) {
            setDebouncedBit(id, rawPressed);
        }
    }
    
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnRawLast[i] = btnRaw[i];
    }
}

// =============================================================================
// API
// =============================================================================

static bool isPressed(uint8_t id) {
    return getDebouncedBit(id);
}

static void setLED(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT) return;
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = (id - 1) % 8;
    if (on) {
        ledState[ic] |= (1 << bit);
    } else {
        ledState[ic] &= ~(1 << bit);
    }
}

// =============================================================================
// Debug
// =============================================================================

static uint8_t btnDebouncedLast[BTN_BYTES];

static bool debouncedChanged() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnDebounced[i] != btnDebouncedLast[i]) return true;
    }
    return false;
}

static bool anyDebouncedPressed() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnDebounced[i] != 0xFF) return true;
    }
    return false;
}

static void printBinary(uint8_t* arr, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) {
        if (i > 0) Serial.print(" ");
        for (int8_t bit = 7; bit >= 0; --bit) {
            Serial.print((arr[i] >> bit) & 1);
        }
    }
}

static void printState() {
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (isPressed(id)) {
            Serial.print("T");
            if (id < 10) Serial.print(" ");
            Serial.print(id);
            Serial.print(" PRESSED | RAW:");
            printBinary(btnRaw, BTN_BYTES);
            Serial.print(" DEB:");
            printBinary(btnDebounced, BTN_BYTES);
            Serial.println();
        }
    }
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 5 Simple: Zeitbasiertes Debouncing");
    Serial.println("========================================");
    Serial.println();
    Serial.print("Debounce-Zeit: ");
    Serial.print(DEBOUNCE_MS);
    Serial.println(" ms");
    Serial.println();
    
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_BTN_PS, OUTPUT);
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
    pinMode(PIN_LED_MOSI, OUTPUT);
    pinMode(PIN_LED_RCK, OUTPUT);
    digitalWrite(PIN_SCK, LOW);
    digitalWrite(PIN_BTN_PS, LOW);
    digitalWrite(PIN_LED_RCK, LOW);
    
    ledcSetup(LEDC_CHANNEL, PWM_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
    setLEDBrightness(PWM_DUTY);
    
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnRaw[i] = 0xFF;
        btnRawLast[i] = 0xFF;
        btnDebounced[i] = 0xFF;
        btnDebouncedLast[i] = 0xFF;
        ledState[i] = 0x00;
    }
    for (size_t i = 0; i < BTN_COUNT; ++i) {
        btnLastChange[i] = 0;
    }
    
    writeLEDs();
    Serial.println("Bereit...");
    Serial.println();
}

// =============================================================================
// Loop
// =============================================================================

void loop() {
    readButtons();
    debounceButtons();
    
    for (uint8_t id = 1; id <= LED_COUNT; ++id) {
        setLED(id, isPressed(id));
    }
    writeLEDs();
    
    if (debouncedChanged() && anyDebouncedPressed()) {
        printState();
    }
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnDebouncedLast[i] = btnDebounced[i];
    }
    
    delay(LOOP_DELAY_MS);
}
