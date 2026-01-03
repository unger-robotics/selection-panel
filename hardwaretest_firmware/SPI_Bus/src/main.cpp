#include <Arduino.h>
#include <SPI.h>

/*
  Gemeinsamer Hardware-SPI-Bus für:
  - 74HC595 LED-Kette (Write: MOSI+SCK, Übernahme: LATCH_595)
  - CD4021 Taster-Kette (Read: MISO+SCK, Parallel-Load: LOAD_4021)

  XIAO ESP32-S3 (Seeed) – Hardware-SPI:
  - D8  = SCK
  - D9  = MISO
  - D10 = MOSI

  Du willst:
  - D0 = LATCH_595 (RCLK beim 74HC595)
  - D1 = LOAD_4021 (P/S beim CD4021)

  Warum gemeinsamer SCK funktioniert:
  - Beim Tasterlesen taktet SCK auch die 74HC595-Schieberegister, aber OHNE
  LATCH-Flanke ändern sich die LED-Ausgänge nicht.
  - Beim LED-Schreiben taktet SCK auch die CD4021, aber vor jedem Lesen wird per
  LOAD der echte Tasterzustand wieder parallel geladen.

  Externe Pull-ups (10 kΩ) nach 3.3 V, Taster nach GND:
  - active-low: gedrückt = 0

  WICHTIG für lineare Taster-Nummerierung ohne Lookup:
  Pro CD4021 IC #k musst du so verdrahten:
    BTN(8k+1) -> PI-8, BTN(8k+2) -> PI-7, ... BTN(8k+8) -> PI-1
  Dann gilt: Bit0 im gelesenen Byte entspricht BTN(8k+1).
*/

// ---------- Pinbelegung ----------
constexpr int PIN_LATCH_595 = D0; // 74HC595 RCLK (Pin 12)
constexpr int PIN_LOAD_4021 = D1; // CD4021  P/S  (Pin 9)

constexpr int PIN_SPI_SCK = D8;   // SCK
constexpr int PIN_SPI_MISO = D9;  // MISO <- CD4021 Q8 (Pin 3) von IC#0
constexpr int PIN_SPI_MOSI = D10; // MOSI -> 74HC595 SER (Pin 14)

// ---------- Projektgröße (Prototyp 10; später 100) ----------
constexpr uint16_t NUM_LEDS = 10; // später: 100
constexpr uint16_t NUM_BTNS = 10; // später: 100

constexpr size_t LED_BYTES = (NUM_LEDS + 7) / 8; // 10->2, 100->13
constexpr size_t BTN_BYTES = (NUM_BTNS + 7) / 8; // 10->2, 100->13

static_assert(NUM_LEDS >= 1 && NUM_LEDS <= 104,
              "74HC595: 1..104 LEDs (13 ICs)");
static_assert(NUM_BTNS >= 1 && NUM_BTNS <= 104,
              "CD4021: 1..104 Taster (13 ICs)");
static_assert(LED_BYTES <= 13 && BTN_BYTES <= 13, "Max 13 Bytes (13 ICs)");

// ---------- Logik-Konventionen ----------
constexpr bool LED_ACTIVE_HIGH = true; // true: LED an = 1 (typisch)
constexpr bool BTN_ACTIVE_LOW = true;  // externe Pull-ups => gedrückt = 0

// ---------- Lange Leitungen: konservative Parameter ----------
constexpr uint32_t SPI_HZ = 1'000'000; // Start 0.5..1 MHz
constexpr uint8_t LOAD_SETUP_US = 2;   // Setup um LOAD
constexpr uint8_t LATCH_SETUP_US = 1;  // Setup um LATCH
constexpr uint8_t LATCH_HOLD_US = 1;   // Hold um LATCH
constexpr uint8_t SCK_PULSE_US = 2;    // "Commit"-Puls beim Parallel-Load

// ---------- Scan + Entprellen ----------
constexpr uint32_t SCAN_MS = 5;      // robust für lange Leitungen
constexpr uint32_t DEBOUNCE_MS = 50; // 20..50 ms (hier 50)

constexpr uint16_t DEBOUNCE_TICKS = (DEBOUNCE_MS + SCAN_MS - 1) / SCAN_MS;
static_assert(DEBOUNCE_TICKS >= 1, "DEBOUNCE_TICKS muss >= 1 sein");

// ---------- Zustände ----------
uint8_t ledBytes[LED_BYTES] = {0};
uint8_t btnRaw[BTN_BYTES] = {0};    // Bit=1 => gedrückt (logisch)
uint8_t btnStable[BTN_BYTES] = {0}; // entprellt
uint8_t debounceCnt[NUM_BTNS] = {0};

bool ledsDirty = true;

// ---------- Hilfsfunktionen ----------
inline void maskUnused(uint8_t *a, size_t bytes, uint16_t nBits) {
    const uint8_t rem = uint8_t(nBits % 8);
    if (rem != 0) {
        const uint8_t mask = uint8_t((1u << rem) - 1u);
        a[bytes - 1] &= mask;
    }
}

inline bool getBit1(const uint8_t *a, uint16_t idx1) {
    const uint16_t idx0 = idx1 - 1;
    const size_t b = idx0 / 8;
    const uint8_t bit = uint8_t(idx0 % 8);
    return (a[b] & uint8_t(1u << bit)) != 0;
}

inline void setBit1(uint8_t *a, uint16_t idx1, bool v) {
    const uint16_t idx0 = idx1 - 1;
    const size_t b = idx0 / 8;
    const uint8_t bit = uint8_t(idx0 % 8);
    const uint8_t m = uint8_t(1u << bit);
    if (v)
        a[b] |= m;
    else
        a[b] &= ~m;
}

// Ein sauberer SCK-Puls außerhalb der SPI-Transaction (für LOAD-Commit).
inline void sckCommitPulse() {
    digitalWrite(PIN_SPI_SCK, HIGH);
    delayMicroseconds(SCK_PULSE_US);
    digitalWrite(PIN_SPI_SCK, LOW);
    delayMicroseconds(SCK_PULSE_US);
}

// ---------- LED-API (74HC595) ----------
inline void ledsClearAll() {
    const uint8_t fill = LED_ACTIVE_HIGH ? 0x00 : 0xFF;
    for (size_t i = 0; i < LED_BYTES; ++i)
        ledBytes[i] = fill;
    ledsDirty = true;
}

// LED Index 1..NUM_LEDS (linear, skalierbar)
inline void ledsSet(uint16_t ledIndex1, bool on) {
    if (ledIndex1 < 1 || ledIndex1 > NUM_LEDS)
        return;

    const uint16_t idx0 = ledIndex1 - 1;
    const size_t ic = idx0 / 8;
    const uint8_t bit = uint8_t(idx0 % 8); // Q0..Q7
    const uint8_t mask = uint8_t(1u << bit);

    const bool driveHigh = LED_ACTIVE_HIGH ? on : !on;
    if (driveHigh)
        ledBytes[ic] |= mask;
    else
        ledBytes[ic] &= ~mask;

    ledsDirty = true;
}

inline void ledsSetOnly(uint16_t ledIndex1) {
    ledsClearAll();
    ledsSet(ledIndex1, true);
}

// Push: IC hinten -> vorne (IC #0 am ESP wird zuletzt gesendet)
inline void ledsShowIfDirty() {
    if (!ledsDirty)
        return;

    digitalWrite(PIN_LATCH_595, LOW);
    delayMicroseconds(LATCH_SETUP_US);

    SPI.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
    for (size_t i = LED_BYTES; i-- > 0;) {
        SPI.transfer(ledBytes[i]);
    }
    SPI.endTransaction();

    delayMicroseconds(LATCH_HOLD_US);
    digitalWrite(PIN_LATCH_595, HIGH);
    delayMicroseconds(LATCH_HOLD_US);

    ledsDirty = false;
}

// ---------- Taster-API (CD4021) ----------
inline void buttonsReadRaw() {
    // 1) Parallel-Load: P/S HIGH, dann ein "Commit"-Clock-Puls, dann P/S LOW.
    // Warum Commit-Puls: robust gegen Grenzfälle/Timing bei langen Leitungen.
    digitalWrite(PIN_SPI_SCK, LOW);
    digitalWrite(PIN_LOAD_4021, HIGH);
    delayMicroseconds(LOAD_SETUP_US);
    sckCommitPulse();
    digitalWrite(PIN_LOAD_4021, LOW);
    delayMicroseconds(LOAD_SETUP_US);

    // 2) Auslesen: LSBFIRST => erstes serielles Bit wird Bit0 im Byte.
    // Mit deiner Verdrahtung (BTN(8k+1)->PI-8...) wird Bit0 = BTN(8k+1).
    SPI.beginTransaction(SPISettings(SPI_HZ, LSBFIRST, SPI_MODE0));
    for (size_t ic = 0; ic < BTN_BYTES; ++ic) {
        const uint8_t raw = SPI.transfer(0x00); // dummy MOSI, wir lesen MISO
        btnRaw[ic] = BTN_ACTIVE_LOW ? uint8_t(~raw) : raw; // gedrückt => 1
    }
    SPI.endTransaction();

    maskUnused(btnRaw, BTN_BYTES, NUM_BTNS);
}

inline void onButtonEvent(uint16_t id1, bool pressed) {
    Serial.print(pressed ? "PRESS " : "RELEASE ");
    Serial.println(id1);

    // Demo-Verhalten: gedrückter Taster schaltet „seine“ LED exklusiv ein
    if (pressed && id1 <= NUM_LEDS) {
        ledsSetOnly(id1);
    }
}

inline void buttonsDebounceAndEmit() {
    for (uint16_t i = 1; i <= NUM_BTNS; ++i) {
        const bool raw = getBit1(btnRaw, i);
        const bool stable = getBit1(btnStable, i);

        // identisch => Zähler zurücksetzen
        if (raw == stable) {
            debounceCnt[i - 1] = 0;
            continue;
        }

        // abweichend => hochzählen bis Schwelle
        uint16_t c = debounceCnt[i - 1];
        if (c < DEBOUNCE_TICKS)
            c++;
        debounceCnt[i - 1] =
            uint8_t(c); // DEBOUNCE_TICKS ist klein (typisch <= 50)

        if (c >= DEBOUNCE_TICKS) {
            setBit1(btnStable, i, raw);
            debounceCnt[i - 1] = 0;
            onButtonEvent(i, raw);
        }
    }
}

// ---------- Zyklus: alles in einer Update-Funktion ----------
inline void updateIO() {
    static uint32_t t0 = 0;
    const uint32_t now = millis();

    if (now - t0 >= SCAN_MS) {
        t0 = now;
        buttonsReadRaw();
        buttonsDebounceAndEmit();
    }

    ledsShowIfDirty();
}

// ---------- Arduino entry ----------
void setup() {
    Serial.begin(115200);

    pinMode(PIN_LATCH_595, OUTPUT);
    pinMode(PIN_LOAD_4021, OUTPUT);
    digitalWrite(PIN_LATCH_595, HIGH);
    digitalWrite(PIN_LOAD_4021, LOW);

    // SPI initialisieren (SCK/MISO/MOSI fest vorgeben)
    SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI);

    // SCK als definierter GPIO für den Load-Commit-Puls
    pinMode(PIN_SPI_SCK, OUTPUT);
    digitalWrite(PIN_SPI_SCK, LOW);

    // Start: LEDs aus, Tasterzustand als „stabil“ übernehmen (kein
    // Startup-Spam)
    ledsClearAll();
    ledsShowIfDirty();

    buttonsReadRaw();
    for (size_t k = 0; k < BTN_BYTES; ++k)
        btnStable[k] = btnRaw[k];
    maskUnused(btnStable, BTN_BYTES, NUM_BTNS);
}

void loop() { updateIO(); }
