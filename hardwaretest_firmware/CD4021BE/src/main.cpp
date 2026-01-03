#include <Arduino.h>

/*
  CD4021 Button-Chain (kaskadiert) – Bitbang-Clock, stabil & einfach

  Externe Pull-ups (10k) nach 3.3V, Taster nach GND:
  => active-low: gedrückt = 0

  Verdrahtung pro IC #k (für lineare Zuordnung ohne Mapping):
    BTN(8k+1) -> PI-8 (Pin 1)
    BTN(8k+2) -> PI-7 (Pin 15)
    BTN(8k+3) -> PI-6 (Pin 14)
    BTN(8k+4) -> PI-5 (Pin 13)
    BTN(8k+5) -> PI-4 (Pin 4)
    BTN(8k+6) -> PI-3 (Pin 5)
    BTN(8k+7) -> PI-2 (Pin 6)
    BTN(8k+8) -> PI-1 (Pin 7)

  Kaskade (IC #0 am ESP):
    Q8 (Pin 3) IC#0 -> BTN_DATA_PIN
    Q8 IC#1 -> SER (Pin 11) IC#0
    ...
    SER (Pin 11) letzter IC -> 3.3V (oder GND), aber NICHT floaten

  Steuerpins:
    P/S (Pin 9): HIGH = Parallel Load, LOW = Shift
    CLK (Pin 10): positive Flanke schiebt weiter

  Warum CLK_HALF_US?
    Lange Leitungen => Flanken beruhigen / weniger Übersprechen / robustere
  Sampling-Zeit.
*/

constexpr int BTN_DATA_PIN = 8; // GPIO8  <- Q8 (Pin 3) von IC #0
constexpr int BTN_CLK_PIN = 7;  // GPIO7  -> CLK (Pin 10) Bus
constexpr int BTN_LOAD_PIN = 6; // GPIO6  -> P/S (Pin 9) Bus

constexpr bool BTN_ACTIVE_LOW = true;

// Prototyp: 10; später: 100
constexpr uint16_t NUM_BTNS = 10;                // später: 100
constexpr size_t BTN_BYTES = (NUM_BTNS + 7) / 8; // 10->2, 100->13

static_assert(NUM_BTNS >= 1, "NUM_BTNS muss >= 1 sein");
static_assert(NUM_BTNS <= 104, "13xCD4021 -> max. 104 Buttons");
static_assert(BTN_BYTES <= 13, "Maximal 13 Bytes (13xCD4021) unterstützt");
static_assert(NUM_BTNS != 100 || BTN_BYTES == 13,
              "Bei 100 Buttons muss BTN_BYTES = 13 sein");

// Scan + Debounce
constexpr uint32_t SCAN_MS =
    5; // stabiler Startwert für lange Leitungen + viele ICs
constexpr uint32_t DEBOUNCE_MS = 50; // 20..50 ms (hier 50 ms)

constexpr uint16_t DEBOUNCE_TICKS =
    (DEBOUNCE_MS + SCAN_MS - 1) / SCAN_MS; // ceil
static_assert(DEBOUNCE_TICKS >= 1, "DEBOUNCE_TICKS muss >= 1 sein");

constexpr uint8_t CLK_HALF_US =
    5; // lange Leitungen: 5..10 µs sind meist robust
constexpr uint8_t LOAD_SETUP_US = 2;

uint8_t btnRaw[BTN_BYTES] = {0};    // Bit=1 => gedrückt (logisch)
uint8_t btnStable[BTN_BYTES] = {0}; // entprellt, Bit=1 => gedrückt
uint8_t dbCnt[NUM_BTNS] = {0};      // pro Taste Zähler (0..DEBOUNCE_TICKS)

inline void clockPulse() {
    digitalWrite(BTN_CLK_PIN, HIGH);
    delayMicroseconds(CLK_HALF_US);
    digitalWrite(BTN_CLK_PIN, LOW);
    delayMicroseconds(CLK_HALF_US);
}

// Bitzugriff (Index 1..NUM_BTNS)
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

// 1) Rohscan: btnRaw[ic] Bit0..7 => BTN(8*ic+1)..BTN(8*ic+8)
inline void buttonsUpdateRaw() {
    // Parallel Load (mit "commit"-Puls)
    digitalWrite(BTN_CLK_PIN, LOW);
    digitalWrite(BTN_LOAD_PIN, HIGH);
    delayMicroseconds(LOAD_SETUP_US);
    clockPulse();
    digitalWrite(BTN_LOAD_PIN, LOW);
    delayMicroseconds(LOAD_SETUP_US);

    // Shift & read: vor dem Clock-Puls lesen, damit das erste Bit nicht
    // verloren geht
    for (size_t ic = 0; ic < BTN_BYTES; ++ic) {
        uint8_t out = 0;

        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint8_t raw = digitalRead(BTN_DATA_PIN) ? 1u : 0u;
            const bool pressed = BTN_ACTIVE_LOW ? (raw == 0u) : (raw == 1u);

            if (pressed)
                out |= uint8_t(
                    1u
                    << bit); // Bit0=BTN(8*ic+1) dank Verdrahtung (PI-8..PI-1)
            clockPulse();
        }

        btnRaw[ic] = out;
    }
}

// 2) Entprellen: Zustand muss DEBOUNCE_TICKS Scans stabil abweichen, dann
// übernehmen + Event
inline void buttonsDebounceAndEmit() {
    for (uint16_t i = 1; i <= NUM_BTNS; ++i) {
        const bool raw = getBit1(btnRaw, i);
        const bool stable = getBit1(btnStable, i);

        if (raw == stable) {
            dbCnt[i - 1] = 0;
            continue;
        }

        uint8_t c = dbCnt[i - 1];
        if (c < DEBOUNCE_TICKS)
            c++;
        dbCnt[i - 1] = c;

        if (c >= DEBOUNCE_TICKS) {
            setBit1(btnStable, i, raw);
            dbCnt[i - 1] = 0;

            Serial.print(raw ? "PRESS " : "RELEASE ");
            Serial.println(i);
        }
    }
}

// Public API (entprellt)
inline bool buttonIsPressed(uint16_t btnIndex1) {
    if (btnIndex1 < 1 || btnIndex1 > NUM_BTNS)
        return false;
    return getBit1(btnStable, btnIndex1);
}

void setup() {
    Serial.begin(115200);

    pinMode(BTN_DATA_PIN, INPUT); // externe Pull-ups -> kein INPUT_PULLUP
    pinMode(BTN_CLK_PIN, OUTPUT);
    pinMode(BTN_LOAD_PIN, OUTPUT);

    digitalWrite(BTN_CLK_PIN, LOW);
    digitalWrite(BTN_LOAD_PIN, LOW);

    // Initialer stabiler Zustand
    buttonsUpdateRaw();
    for (size_t k = 0; k < BTN_BYTES; ++k)
        btnStable[k] = btnRaw[k];
}

void loop() {
    static uint32_t t0 = 0;
    const uint32_t now = millis();

    if (now - t0 >= SCAN_MS) {
        t0 = now;
        buttonsUpdateRaw();
        buttonsDebounceAndEmit();
    }
}
