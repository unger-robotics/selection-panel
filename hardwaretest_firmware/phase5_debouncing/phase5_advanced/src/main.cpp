/**
 * @file main.cpp
 * @brief Phase 5 Advanced: State Machine mit Events
 * @version 1.0.0
 * 
 * Professionelles Debouncing mit State Machine:
 * - PRESS Event (einmalig beim Drücken)
 * - RELEASE Event (einmalig beim Loslassen)
 * - Toggle-Modus für LEDs
 */

#include <Arduino.h>
#include "config.h"

// =============================================================================
// Button State Machine
// =============================================================================

enum class BtnState : uint8_t {
    IDLE,       // Nicht gedrückt
    PRESSING,   // Übergang zu gedrückt
    PRESSED,    // Gedrückt
    RELEASING   // Übergang zu nicht gedrückt
};

enum class BtnEvent : uint8_t {
    NONE,
    PRESS,
    RELEASE
};

struct Button {
    BtnState state;
    uint32_t lastChange;
    bool ledOn;
};

static Button buttons[BTN_COUNT];
static uint8_t btnRaw[BTN_BYTES];
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
// Button API
// =============================================================================

static bool getRawPressed(uint8_t id) {
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = 7 - ((id - 1) % 8);
    return !(btnRaw[ic] & (1 << bit));
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
// State Machine
// =============================================================================

static BtnEvent updateButton(uint8_t id) {
    Button& btn = buttons[id - 1];
    bool rawPressed = getRawPressed(id);
    uint32_t now = millis();
    BtnEvent event = BtnEvent::NONE;
    
    switch (btn.state) {
        case BtnState::IDLE:
            if (rawPressed) {
                btn.state = BtnState::PRESSING;
                btn.lastChange = now;
            }
            break;
            
        case BtnState::PRESSING:
            if (!rawPressed) {
                btn.state = BtnState::IDLE;
            } else if (now - btn.lastChange >= DEBOUNCE_MS) {
                btn.state = BtnState::PRESSED;
                event = BtnEvent::PRESS;
            }
            break;
            
        case BtnState::PRESSED:
            if (!rawPressed) {
                btn.state = BtnState::RELEASING;
                btn.lastChange = now;
            }
            break;
            
        case BtnState::RELEASING:
            if (rawPressed) {
                btn.state = BtnState::PRESSED;
            } else if (now - btn.lastChange >= DEBOUNCE_MS) {
                btn.state = BtnState::IDLE;
                event = BtnEvent::RELEASE;
            }
            break;
    }
    
    return event;
}

// =============================================================================
// Event Callbacks
// =============================================================================

static void onPress(uint8_t id) {
    buttons[id - 1].ledOn = !buttons[id - 1].ledOn;
    setLED(id, buttons[id - 1].ledOn);
    
    Serial.print("T");
    if (id < 10) Serial.print(" ");
    Serial.print(id);
    Serial.print(" PRESS   -> LED ");
    Serial.println(buttons[id - 1].ledOn ? "ON" : "OFF");
}

static void onRelease(uint8_t id) {
    Serial.print("T");
    if (id < 10) Serial.print(" ");
    Serial.print(id);
    Serial.println(" RELEASE");
}

// =============================================================================
// Setup
// =============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {}
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("Phase 5 Advanced: State Machine Events");
    Serial.println("========================================");
    Serial.println();
    Serial.print("Debounce-Zeit: ");
    Serial.print(DEBOUNCE_MS);
    Serial.println(" ms");
    Serial.println("Modus: Toggle (Taster = LED umschalten)");
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
    
    for (size_t i = 0; i < BTN_COUNT; ++i) {
        buttons[i].state = BtnState::IDLE;
        buttons[i].lastChange = 0;
        buttons[i].ledOn = false;
    }
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnRaw[i] = 0xFF;
        ledState[i] = 0x00;
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
    
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        BtnEvent event = updateButton(id);
        
        if (event == BtnEvent::PRESS) {
            onPress(id);
        } else if (event == BtnEvent::RELEASE) {
            onRelease(id);
        }
    }
    
    writeLEDs();
    delay(LOOP_DELAY_MS);
}
