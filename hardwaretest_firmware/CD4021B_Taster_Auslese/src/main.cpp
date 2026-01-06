/**
 * @file main.cpp
 * @brief CD4021B Taster-Auslese – 10 Taster
 *
 * Verdrahtung (beide ICs identisch):
 *   IC0: Taster 1-8 an P8-P1 (P8=T1, P7=T2, ..., P1=T8)
 *   IC1: Taster 9-16 an P8-P1 (P8=T9, P7=T10, ..., P1=T16)
 *   Genutzt: nur Taster 1-10
 */

#include <Arduino.h>

// Pin-Konfiguration
constexpr int PIN_PS_4021 = D1;
constexpr int PIN_SPI_SCK = D8;
constexpr int PIN_SPI_MISO = D9;

// Button-Konfiguration
constexpr size_t BTN_BYTES = 2;
constexpr size_t BTN_COUNT = 10;

// Lookup-Tabelle: [IC][Bit-Index] → Taster-ID (0 = nicht belegt)
// Bit-Index 0 = Bit7 (MSB, Q8/P8), Bit-Index 7 = Bit0 (LSB, Q1/P1)
constexpr uint8_t BTN_MAP[BTN_BYTES][8] = {
    // IC0: P8=T1, P7=T2, P6=T3, P5=T4, P4=T5, P3=T6, P2=T7, P1=T8
    {1, 2, 3, 4, 5, 6, 7, 8},

    // IC1: P8=T9, P7=T10, Rest unbelegt (nur 10 Taster genutzt)
    {9, 10, 0, 0, 0, 0, 0, 0}};

// Zustand für Änderungserkennung
static uint8_t lastState[BTN_BYTES] = {0xFF, 0xFF};

// -----------------------------------------------------------------------------
// Manueller Clock-Puls
// -----------------------------------------------------------------------------
static inline void clockPulse() {
    digitalWrite(PIN_SPI_SCK, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_SPI_SCK, LOW);
    delayMicroseconds(5);
}

// -----------------------------------------------------------------------------
// CD4021B auslesen (Bit-Banging)
// -----------------------------------------------------------------------------
static void read4021(uint8_t *out, size_t nBytes) {
    // Parallel-Load
    digitalWrite(PIN_PS_4021, HIGH);
    delayMicroseconds(5);
    digitalWrite(PIN_PS_4021, LOW);
    delayMicroseconds(2);

    // Bit-Banging: Q8 liegt bereits am Ausgang
    for (size_t i = 0; i < nBytes; ++i) {
        uint8_t byte = 0;

        for (int8_t bit = 7; bit >= 0; --bit) {
            if (digitalRead(PIN_SPI_MISO)) {
                byte |= (1 << bit);
            }
            clockPulse();
        }

        out[i] = byte;
    }
}

// -----------------------------------------------------------------------------
// Gedrückte Taster ausgeben
// -----------------------------------------------------------------------------
static void printButtons(const uint8_t *data) {
    Serial.print("Taster: ");
    bool any = false;

    for (size_t ic = 0; ic < BTN_BYTES; ++ic) {
        for (uint8_t idx = 0; idx < 8; ++idx) {
            uint8_t id = BTN_MAP[ic][idx];

            if (id == 0)
                continue; // Nicht belegt

            // idx=0 → Bit7, idx=1 → Bit6, ..., idx=7 → Bit0
            uint8_t mask = uint8_t(1u << (7 - idx));

            if (!(data[ic] & mask)) { // LOW = gedrückt
                if (any)
                    Serial.print(", ");
                Serial.print(id);
                any = true;
            }
        }
    }

    if (!any)
        Serial.print("-");
    Serial.println();
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
    }

    pinMode(PIN_PS_4021, OUTPUT);
    digitalWrite(PIN_PS_4021, LOW);

    pinMode(PIN_SPI_SCK, OUTPUT);
    digitalWrite(PIN_SPI_SCK, LOW);

    pinMode(PIN_SPI_MISO, INPUT_PULLUP);

    Serial.println("CD4021B Ready (10 Taster)");
    Serial.println("IC0: T1-T8 | IC1: T9-T10");
    Serial.println("───────────────────────────");
}

// -----------------------------------------------------------------------------
// Loop
// -----------------------------------------------------------------------------
void loop() {
    uint8_t state[BTN_BYTES];
    read4021(state, BTN_BYTES);

    // Nur bei Änderung ausgeben
    if (state[0] != lastState[0] || state[1] != lastState[1]) {
        printButtons(state);
        lastState[0] = state[0];
        lastState[1] = state[1];
    }

    delay(20);
}
