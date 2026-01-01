#include <Arduino.h>

/*
  Target: Seeed Studio XIAO ESP32S3 (ESP32-S3, Arduino/PlatformIO)

  Hardware: 2× 74HC595 (kaskadiert) -> 10 LEDs (bei dir: LED1 links … LED10
  rechts)

  Pin-Mapping (XIAO-Board):
  - D0 -> GPIO 1 : DATA  (SER / 74HC595 Pin 14)
  - D1 -> GPIO 2 : CLOCK (SRCLK / 74HC595 Pin 11)
  - D2 -> GPIO 3 : LATCH (RCLK / 74HC595 Pin 12)

  Logikpegel:
  - ESP32-S3: 3,3 V I/O
  - 74HC595 daher mit 3,3 V versorgen (wie in deinem Schaltplan), dann ist VIH
  sicher erfüllt. (Bei 5 V Versorgung bräuchtest du Pegelwandler/geeignete
  Logikfamilie.)

  Ziel:
  - Simple is best: eine HW-Funktion write595()
  - Keine Redundanz: Zustände als Konstanten
  - Wartbar: definierte Startzustände, keine delay()-Blockade
  - Skalierbar: klare Byte-Reihenfolge (U2 zuerst, dann U1)

  Daten-Reihenfolge (Kaskade):
  - Erstes Byte landet im hinteren Chip (U2),
    zweites Byte bleibt im vorderen Chip (U1, am ESP).
*/

constexpr uint8_t DATA_PIN = 1;  // SER   (74HC595 Pin 14)
constexpr uint8_t CLOCK_PIN = 2; // SRCLK (74HC595 Pin 11)
constexpr uint8_t LATCH_PIN = 3; // RCLK  (74HC595 Pin 12)

// Zustände
constexpr uint8_t U_ALL_OFF = 0x00;
constexpr uint8_t U1_ON = 0xFF;       // U1: LED1..LED8 an
constexpr uint8_t U2_ON = 0b00000011; // U2: LED9..LED10 an (QA/QB)

constexpr uint32_t BLINK_MS = 1000;

// Schreibt 2 Bytes in die Kette:
// - erst U2 (hinten), dann U1 (am ESP)
inline void write595(uint8_t u2, uint8_t u1) {
    digitalWrite(LATCH_PIN, LOW);                // Ausgänge halten
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, u2); // wandert bis nach U2
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, u1); // bleibt in U1
    digitalWrite(LATCH_PIN, HIGH);               // übernimmt auf QA..QH
}

void setup() {
    pinMode(DATA_PIN, OUTPUT);
    pinMode(CLOCK_PIN, OUTPUT);
    pinMode(LATCH_PIN, OUTPUT);

    // Definierte Startpegel (Glitch-sicherer beim Reset)
    digitalWrite(DATA_PIN, LOW);
    digitalWrite(CLOCK_PIN, LOW);
    digitalWrite(LATCH_PIN, HIGH);

    write595(U_ALL_OFF, U_ALL_OFF);
}

void loop() {
    static uint32_t t0 = 0;
    static bool on = false;

    const uint32_t now = millis();
    if (now - t0 >= BLINK_MS) {
        t0 = now;
        on = !on;

        // EIN/AUS ohne Blockierung
        write595(on ? U2_ON : U_ALL_OFF, on ? U1_ON : U_ALL_OFF);
    }
}
