/**
 * @file main.cpp
 * @brief Selection Panel – Hardware-PWM über OE
 *
 * PWM: LEDC an OE-Pin (Active-LOW)
 * Duty 50% → LED 50% Helligkeit
 */

#include <Arduino.h>

// =============================================================================
// KONFIGURATION
// =============================================================================
constexpr size_t BTN_COUNT = 10;
constexpr size_t LED_COUNT = 10;
constexpr uint8_t PWM_DUTY = 50;    // 0-100%
constexpr uint32_t PWM_FREQ = 1000; // 1 kHz (flimmerfrei)

// Pins
constexpr int PIN_SCK = D8;
constexpr int PIN_BTN_PS = D1;
constexpr int PIN_BTN_MISO = D9;
constexpr int PIN_LED_MOSI = D10;
constexpr int PIN_LED_RCK = D0;
constexpr int PIN_LED_OE = D2; // NEU: Output Enable (PWM)

// LEDC
constexpr uint8_t LEDC_CHANNEL = 0;
constexpr uint8_t LEDC_RESOLUTION = 8; // 8-Bit (0-255)

// =============================================================================
// Abgeleitete Konstanten
// =============================================================================
constexpr size_t BTN_BYTES = (BTN_COUNT + 7) / 8;
constexpr size_t LED_BYTES = (LED_COUNT + 7) / 8;

// =============================================================================
// Zustand
// =============================================================================
static uint8_t btnState[BTN_BYTES];
static uint8_t btnLast[BTN_BYTES];
static uint8_t ledState[LED_BYTES];

// =============================================================================
// Low-Level
// =============================================================================
static inline void clockPulse() {
    digitalWrite(PIN_SCK, HIGH);
    delayMicroseconds(2);
    digitalWrite(PIN_SCK, LOW);
    delayMicroseconds(2);
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
        btnState[ic] = byte;
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

// =============================================================================
// PWM
// =============================================================================
static void setLEDBrightness(uint8_t percent) {
    // OE ist Active-LOW: LOW = LEDs an, HIGH = LEDs aus
    // duty 0 = voll an, duty 255 = aus
    uint8_t duty = 255 - (percent * 255 / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

// =============================================================================
// API
// =============================================================================
static bool isPressed(uint8_t id) {
    if (id < 1 || id > BTN_COUNT)
        return false;
    uint8_t ic = (id - 1) / 8;
    uint8_t bit = 7 - ((id - 1) % 8);
    return !(btnState[ic] & (1 << bit));
}

static void setLED(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT)
        return;
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
static void printPressed() {
    Serial.print("Taster: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (isPressed(id)) {
            if (any)
                Serial.print(", ");
            Serial.print(id);
            any = true;
        }
    }
    if (!any)
        Serial.print("-");
    Serial.println();
}

static bool btnChanged() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnState[i] != btnLast[i])
            return true;
    }
    return false;
}

static void btnSave() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        btnLast[i] = btnState[i];
    }
}

// =============================================================================
// Setup
// =============================================================================
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
    }

    // Pins
    pinMode(PIN_SCK, OUTPUT);
    pinMode(PIN_BTN_PS, OUTPUT);
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
    pinMode(PIN_LED_MOSI, OUTPUT);
    pinMode(PIN_LED_RCK, OUTPUT);

    digitalWrite(PIN_SCK, LOW);
    digitalWrite(PIN_BTN_PS, LOW);
    digitalWrite(PIN_LED_RCK, LOW);

    // LEDC PWM für OE
    ledcSetup(LEDC_CHANNEL, PWM_FREQ, LEDC_RESOLUTION);
    ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
    setLEDBrightness(PWM_DUTY);

    // Init
    for (size_t i = 0; i < BTN_BYTES; ++i)
        btnLast[i] = 0xFF;
    for (size_t i = 0; i < LED_BYTES; ++i)
        ledState[i] = 0x00;

    writeLEDs();

    Serial.print("Selection Panel | PWM: ");
    Serial.print(PWM_DUTY);
    Serial.println("% (Hardware)");
}

// =============================================================================
// Loop
// =============================================================================
void loop() {
    readButtons();

    // Taster → LED
    for (uint8_t id = 1; id <= LED_COUNT; ++id) {
        setLED(id, isPressed(id));
    }

    writeLEDs();

    // Debug
    if (btnChanged()) {
        printPressed();
        btnSave();
    }

    delay(20);
}
