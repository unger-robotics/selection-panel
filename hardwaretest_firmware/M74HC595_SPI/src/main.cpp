#include <Arduino.h>
#include <SPI.h>

/*
  74HC595 LED-Chain (kaskadiert), optimiert für lange Leitungen

  Ziel:
  - minimal, konsistent, wartbar, skalierbar (10 -> 100 LEDs)
  - nicht-blockierend (Timing via millis)
  - klare Byte-Reihenfolge: ledBytes[i] entspricht IC #i

  Reale IC-Reihenfolge (dein Standard):
  - IC #0 (direkt am ESP)  = LED1..LED8   (Q0..Q7)
  - ...
  - IC #(NUM_BYTES-1)      = letzte 8 LEDs im Bundle

  Lange Leitungen (Praxis):
  - SPI-Takt reduzieren (Start: 1 MHz, ggf. 500 kHz)
  - kleine Setup/Hold-Delays um LATCH (saubere Übernahme, weniger Glitches)
  - Hardware: nahe am ESP 33–100 Ω Serienwiderstand in SCK und MOSI (optional
  auch LATCH)
*/

constexpr int LATCH_PIN = 3; // GPIO3 (RCLK / 74HC595 Pin 12)

// XIAO ESP32S3 Hardware-SPI Pins (GPIO)
constexpr int SPI_SCK = 7;  // D8  (SRCLK / 74HC595 Pin 11)
constexpr int SPI_MISO = 8; // D9  (ungenutzt bei 74HC595, aber ok)
constexpr int SPI_MOSI = 9; // D10 (SER  / 74HC595 Pin 14)

// Prototyp: 10 LEDs; später: 100
constexpr uint16_t NUM_LEDS = 10;                // später: 100
constexpr size_t NUM_BYTES = (NUM_LEDS + 7) / 8; // 10->2, 100->13

static_assert(NUM_LEDS >= 1, "NUM_LEDS muss >= 1 sein");
static_assert(NUM_LEDS <= 104, "13x74HC595 -> max. 104 LEDs");
static_assert(NUM_BYTES <= 13, "Maximal 13 Bytes (13x74HC595) unterstützt");
static_assert(NUM_LEDS != 100 || NUM_BYTES == 13,
              "Bei 100 LEDs muss NUM_BYTES = 13 sein");

// Logik: je nach Verdrahtung ggf. invertieren
constexpr bool LED_ACTIVE_HIGH = true;

// Lange Leitungen: konservativer SPI-Takt
constexpr uint32_t SPI_HZ = 1'000'000; // Startwert
// constexpr uint32_t SPI_HZ = 500'000;  // falls noch instabil

// LATCH-Timing (µs): klein halten, aber >0 für saubere Flanken/Setup/Hold
constexpr uint8_t LATCH_SETUP_US = 1;
constexpr uint8_t LATCH_HOLD_US = 1;

// ledBytes[i] entspricht IC #i (IC #0 am ESP)
uint8_t ledBytes[NUM_BYTES] = {0};

inline void spiInit() { SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI); }

/*
  Schiebt die Bytes in die 74HC595-Kette (hinten -> vorne).
  Warum rückwärts?
  -> Das zuerst geschobene Byte landet ganz hinten in der Kette.
*/
inline void write595ChainSPI(const uint8_t *bytes, size_t n) {
    if (n == 0)
        return;

    digitalWrite(LATCH_PIN, LOW);      // Ausgänge halten
    delayMicroseconds(LATCH_SETUP_US); // Setup-Zeit (Leitung/Flanke beruhigen)

    SPI.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));

    // IC#(n-1) zuerst (hinten), IC#0 zuletzt (vorn/am ESP)
    for (size_t i = n; i-- > 0;) {
        SPI.transfer(bytes[i]);
    }

    SPI.endTransaction();

    delayMicroseconds(LATCH_HOLD_US); // Hold-Zeit vor Latch
    digitalWrite(LATCH_PIN, HIGH);    // übernimmt gleichzeitig auf QA..QH
    delayMicroseconds(LATCH_HOLD_US); // Hold-Zeit nach Latch (optional)
}

inline void showLeds() { write595ChainSPI(ledBytes, NUM_BYTES); }

inline void clearAllLeds() {
    const uint8_t fill = LED_ACTIVE_HIGH ? 0x00 : 0xFF;
    for (size_t i = 0; i < NUM_BYTES; ++i)
        ledBytes[i] = fill;
}

/*
  Setzt eine LED (1..NUM_LEDS).
  Byte/Bit-Adressierung:
  - idx0 = ledIndex1-1
  - ic   = idx0/8   (0..NUM_BYTES-1) -> entspricht IC #ic
  - bit  = idx0%8   (0..7)           -> Q0..Q7
*/
inline void setLed(uint16_t ledIndex1, bool on) {
    if (ledIndex1 < 1 || ledIndex1 > NUM_LEDS)
        return;

    const uint16_t idx0 = ledIndex1 - 1;
    const size_t ic = idx0 / 8;
    const uint8_t bit = uint8_t(idx0 % 8);
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
    pinMode(LATCH_PIN, OUTPUT);
    digitalWrite(LATCH_PIN, HIGH);

    spiInit();

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
