#include <Arduino.h>

/*
  74HC595 LED-Chain (kaskadiert) – Prototyp 10 LEDs, skalierbar bis 100 LEDs

  Ziel:
  - einheitliches Datenmodell: ledBytes[] statt "u1/u2"
  - lineare Zuordnung: LED n -> (IC=(n-1)/8, Bit=(n-1)%8)
  - nicht-blockierend: Demo mit millis()
  - Kommentare erklären warum: Reihenfolge in der Kette, Bitbedeutung

  Hardware (74HC595):
  - SER   (Pin 14)  <- DATA_PIN
  - SRCLK (Pin 11)  <- CLOCK_PIN
  - RCLK  (Pin 12)  <- LATCH_PIN
  - OE    (Pin 13)  -> GND   (aktiv, Ausgänge immer an)
  - SRCLR (Pin 10)  -> VCC   (kein Reset)
  - QH'   (Pin 9)   -> SER des nächsten IC

  Warum "hinten -> vorne" senden?
  - Das zuerst geschobene Byte wandert durch alle ICs bis ans Ende der Kette.
  - Das zuletzt geschobene Byte bleibt im IC direkt am ESP (IC #0).
*/

constexpr uint8_t DATA_PIN = 1;  // GPIO1  -> SER   (Pin 14)
constexpr uint8_t CLOCK_PIN = 2; // GPIO2  -> SRCLK (Pin 11)
constexpr uint8_t LATCH_PIN = 3; // GPIO3  -> RCLK  (Pin 12)

// Prototyp: 10 LEDs; später: 100
constexpr uint16_t NUM_LEDS = 10;                // später: 100
constexpr size_t NUM_BYTES = (NUM_LEDS + 7) / 8; // 10->2, 100->13

static_assert(NUM_LEDS >= 1, "NUM_LEDS muss >= 1 sein");
static_assert(NUM_LEDS <= 104, "13x74HC595 -> max. 104 LEDs");
static_assert(NUM_BYTES <= 13, "Maximal 13 Bytes (13x74HC595) unterstützt");
static_assert(NUM_LEDS != 100 || NUM_BYTES == 13,
              "Bei 100 LEDs muss NUM_BYTES = 13 sein");

// Falls deine LED-Hardware invertiert ist (z.B. LED an GND mit Vorwiderstand
// nach VCC):
constexpr bool LED_ACTIVE_HIGH = true;

// ledBytes[ic] entspricht IC #ic (IC #0 am ESP, IC #(NUM_BYTES-1) am Ende)
uint8_t ledBytes[NUM_BYTES] = {0};

inline void latchLow() { digitalWrite(LATCH_PIN, LOW); }
inline void latchHigh() { digitalWrite(LATCH_PIN, HIGH); }

/*
  Schiebt die gesamte Kette raus.
  Bitbedeutung bei MSBFIRST:
  - Arduino shiftOut(MSBFIRST) sendet Bit7 zuerst, dann ... Bit0 zuletzt.
  - Beim 74HC595 landet das zuletzt geschobene Bit im Q0.
  => Q0 ↔ Bit0, Q7 ↔ Bit7 (intuitiv, gut dokumentierbar)
*/
inline void write595ChainShiftOut(const uint8_t *bytes, size_t n) {
    if (n == 0)
        return;

    latchLow(); // Ausgänge halten, während wir die Shiftregister befüllen

    // IC#(n-1) zuerst (hinten), IC#0 zuletzt (vorn/am ESP)
    for (size_t i = n; i-- > 0;) {
        shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bytes[i]);
    }

    latchHigh(); // übernimmt alle 8* n Bits gleichzeitig auf QA..QH
}

inline void showLeds() { write595ChainShiftOut(ledBytes, NUM_BYTES); }

inline void clearAllLeds() {
    const uint8_t fill = LED_ACTIVE_HIGH ? 0x00 : 0xFF;
    for (size_t i = 0; i < NUM_BYTES; ++i)
        ledBytes[i] = fill;
}

inline void setLed(uint16_t ledIndex1, bool on) {
    if (ledIndex1 < 1 || ledIndex1 > NUM_LEDS)
        return;

    const uint16_t idx0 = ledIndex1 - 1;
    const size_t ic = idx0 / 8;            // 0..NUM_BYTES-1
    const uint8_t bit = uint8_t(idx0 % 8); // 0..7 (Q0..Q7)
    const uint8_t mask = uint8_t(1u << bit);

    const bool driveHigh = LED_ACTIVE_HIGH ? on : !on;
    if (driveHigh)
        ledBytes[ic] |= mask;
    else
        ledBytes[ic] &= ~mask;
}

inline void setOnlyLed(uint16_t ledIndex1) {
    clearAllLeds();
    setLed(ledIndex1, true);
    showLeds();
}

/*** Demo: Lauflicht (nicht-blockierend) ***/
constexpr uint32_t STEP_MS = 50;

void setup() {
    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);

    // Definierte Startpegel: reduziert "Zucken" beim Boot
    digitalWrite(DATA_PIN, LOW);
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(LATCH_PIN, HIGH);

    clearAllLeds();
    showLeds();
}

void loop() {
    static uint32_t t0 = 0;
    static uint16_t current = 1;

    const uint32_t now = millis();
    if (now - t0 >= STEP_MS) {
        t0 = now;

        setOnlyLed(current);

        current++;
        if (current > NUM_LEDS)
            current = 1;
    }
}
