#include <Arduino.h>

/*
  Target: Seeed Studio XIAO ESP32S3 (PlatformIO + Arduino)
  Version: 1.1.0 (korrigierte CD4021-Leselogik)

  Aufgabe:
  - 2× 74HC595: LEDs (Ausgänge) schalten
  - 2× CD4021BE: Taster (Eingänge) einlesen
  - BTN1 toggelt LED1, ... BTN10 toggelt LED10

  Änderungen v1.1:
  - CD4021: Read-Sequenz korrigiert (erst lesen, dann clocken)
  - Timing-Delays an Datenblatt-Spezifikation angepasst
  - Vorbereitung für Skalierung auf 100×
*/

// =============================================================================
// Konfiguration (ein Ort für Skalierung / Anpassungen)
// =============================================================================
constexpr uint8_t NUM_LEDS = 10;
constexpr uint8_t NUM_BTNS = 10;
constexpr uint8_t SHIFT_BITS = 16;   // 2× 8-bit Shiftregister in Kaskade
constexpr uint32_t DEBOUNCE_MS = 30; // Entprellen (stabiler Zustand)
constexpr uint32_t POLL_MS = 1;      // Begrenze Poll-Rate (CPU-Last/Jitter)

// =============================================================================
// 74HC595 (LED-Ausgänge)
// =============================================================================
// Pins laut Schaltplan: D0/D1/D2
// WICHTIG: OE (active-low) muss extern auf GND liegen, sonst Ausgänge
// hochohmig!
constexpr uint8_t LED_DATA_PIN = D0;  // SER (Pin 14)
constexpr uint8_t LED_CLOCK_PIN = D1; // SRCLK (Pin 11)
constexpr uint8_t LED_LATCH_PIN = D2; // RCLK (Pin 12)

// Bit-Mapping: LEDn (1-basiert) -> Bit-Position im 16-bit Stream
// Bei geänderter Verdrahtung nur hier anpassen.
constexpr uint8_t LED_BIT_MAP[NUM_LEDS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

/*
 * write595() - Schreibt 2 Bytes an die 74HC595-Kaskade
 *
 * Timing laut Datenblatt (74HC595 bei 4.5V):
 * - t_s (Setup SI → SCK): min 5 ns (typ. 10 ns)
 * - t_h (Hold SI nach SCK): 0 ns
 * - t_s (SCK → RCK): min 8 ns (typ. 15 ns)
 *
 * shiftOut() ist langsam genug (~2 µs/bit), alle Timings werden eingehalten.
 *
 * Reihenfolge: u3 (hinteres IC, weiter vom MCU) zuerst, dann u2 (vorderes IC).
 * Latch übernimmt beide Bytes gleichzeitig.
 */
inline void write595(uint8_t u3, uint8_t u2) {
    digitalWrite(LED_LATCH_PIN, LOW);
    shiftOut(LED_DATA_PIN, LED_CLOCK_PIN, MSBFIRST, u3); // IC hinten
    shiftOut(LED_DATA_PIN, LED_CLOCK_PIN, MSBFIRST, u2); // IC vorne (am MCU)
    digitalWrite(LED_LATCH_PIN, HIGH);
}

static uint16_t g_ledState = 0;

inline void applyLeds() {
    const uint8_t u2 = static_cast<uint8_t>(g_ledState & 0xFFu);
    const uint8_t u3 = static_cast<uint8_t>((g_ledState >> 8) & 0xFFu);
    write595(u3, u2);
}

inline void toggleLed(uint8_t ledIndex1Based) {
    if (ledIndex1Based < 1 || ledIndex1Based > NUM_LEDS)
        return;
    const uint8_t bit = LED_BIT_MAP[ledIndex1Based - 1];
    g_ledState ^= (static_cast<uint16_t>(1u) << bit);
}

// =============================================================================
// CD4021BE (Taster-Eingänge)
// =============================================================================
// Pins laut Schaltplan: D3/D4/D5
constexpr uint8_t BTN_DATA_PIN = D3;  // Q8 vom letzten CD4021 (Serial Out)
constexpr uint8_t BTN_CLOCK_PIN = D4; // CLK (Pin 10)
constexpr uint8_t BTN_LOAD_PIN = D5;  // P/S (Pin 9) - HIGH=Load, LOW=Shift

// Taster: Pull-Up nach 3V3, Schalter nach GND => gedrückt = 0 (active-low)
constexpr bool BTN_ACTIVE_LOW = true;

/*
 * Bit-Mapping: Physischer Taster (1..NUM_BTNS) -> Bit-Position im 16-bit Stream
 *
 * Der Stream wird MSB-first aufgebaut: erstes gelesenes Bit = Bit 15.
 * Diese Werte wurden empirisch gemessen und entsprechen der Verdrahtung.
 */
constexpr uint8_t BTN_BIT_MAP[NUM_BTNS] = {
    15, // BTN1  -> Bit 15 (IC#1, Q8)
    12, // BTN2  -> Bit 12 (IC#1, Q5)
    13, // BTN3  -> Bit 13 (IC#1, Q6)
    11, // BTN4  -> Bit 11 (IC#1, Q4)
    10, // BTN5  -> Bit 10 (IC#1, Q3)
    9,  // BTN6  -> Bit 9  (IC#1, Q2)
    8,  // BTN7  -> Bit 8  (IC#1, Q1)
    14, // BTN8  -> Bit 14 (IC#1, Q7)
    7,  // BTN9  -> Bit 7  (IC#0, Q8)
    4   // BTN10 -> Bit 4  (IC#0, Q5)
};

/*
 * read4021Stream16() - Liest 16 Bits von der CD4021-Kaskade
 *
 * Timing laut Datenblatt (CD4021B bei 5V, bei 3.3V konservativer):
 * - t_WH (P/S High Pulse Width): min 80 ns (typ.), 160 ns (max @ 5V)
 * - t_s  (Setup P1-P8 → CLK):   min 25 ns (typ. @ 10V)
 * - t_REM (P/S Removal → CLK):  min 70 ns (typ. @ 10V)
 * - t_W  (Clock Pulse Width):   min 40 ns (typ. @ 10V)
 *
 * KRITISCH: Nach Parallel Load liegt Q8 SOFORT am Ausgang!
 * Sequenz: Load → Lesen → Clock → Lesen → Clock → ... → Lesen (kein Clock am
 * Ende)
 *
 * Wir verwenden 2 µs Delays für Robustheit bei 3.3V (Faktor ~25× Sicherheit).
 */
inline uint16_t read4021Stream16() {
    // 1) Parallel Load: P/S HIGH übernimmt alle Eingänge in die Latches
    digitalWrite(BTN_LOAD_PIN, HIGH);
    delayMicroseconds(2); // t_WH: Load-Puls (min 160 ns @ 5V)
    digitalWrite(BTN_LOAD_PIN, LOW);
    delayMicroseconds(2); // t_REM: Warten bis Q8 stabil (min 140 ns @ 5V)

    // 2) Serial Shift: MSB-first (erstes Bit = Bit 15)
    uint16_t v = 0;
    for (uint8_t i = 0; i < SHIFT_BITS; i++) {
        // ERST lesen (Q8 enthält aktuelles Bit)
        v <<= 1;
        v |= (digitalRead(BTN_DATA_PIN) ? 1u : 0u);

        // DANN clocken (schiebt nächstes Bit nach Q8)
        // Kein Clock nach dem letzten Bit nötig
        if (i < (SHIFT_BITS - 1)) {
            digitalWrite(BTN_CLOCK_PIN, HIGH);
            delayMicroseconds(1); // t_W: Clock High (min 40 ns @ 10V)
            digitalWrite(BTN_CLOCK_PIN, LOW);
            delayMicroseconds(1); // t_W: Clock Low
        }
    }
    return v;
}

/*
 * readButtonsPressedMask() - Wandelt Stream in logische Button-Maske
 *
 * Rückgabe: bit0 = BTN1, bit1 = BTN2, ... bit(n-1) = BTNn
 *           1 = gedrückt, 0 = nicht gedrückt
 */
inline uint16_t readButtonsPressedMask() {
    const uint16_t stream = read4021Stream16();

    uint16_t pressed = 0;
    for (uint8_t i = 0; i < NUM_BTNS; i++) {
        const uint8_t sb = BTN_BIT_MAP[i];
        const bool level = ((stream >> sb) & 1u) != 0;
        const bool isPressed = BTN_ACTIVE_LOW ? (!level) : level;
        if (isPressed)
            pressed |= (static_cast<uint16_t>(1u) << i);
    }
    return pressed;
}

// =============================================================================
// Debounce + Toggle-Logik
// =============================================================================
static uint16_t g_btnRaw = 0;
static uint16_t g_btnStable = 0;
static uint32_t g_btnChangedAt = 0;

/*
 * buttonsTask() - Nicht-blockierendes Debouncing mit Flankenerkennung
 *
 * Prinzip:
 * 1. Rohen Zustand pollen
 * 2. Bei Änderung: Timer neu starten
 * 3. Wenn DEBOUNCE_MS lang stabil: als "gültig" übernehmen
 * 4. Rising Edge (0→1) löst Toggle aus
 */
inline void buttonsTask(uint32_t nowMs) {
    const uint16_t raw = readButtonsPressedMask();

    // Rohänderung: Zeitpunkt merken
    if (raw != g_btnRaw) {
        g_btnRaw = raw;
        g_btnChangedAt = nowMs;
    }

    // Wenn lange genug stabil -> übernehmen und Rising-Edges auswerten
    if ((nowMs - g_btnChangedAt) >= DEBOUNCE_MS && g_btnStable != g_btnRaw) {
        const uint16_t prev = g_btnStable;
        g_btnStable = g_btnRaw;

        // Rising Edge: 0 → 1 (Taste wurde als "gedrückt" erkannt)
        const uint16_t rising = (g_btnStable & ~prev);
        if (rising) {
            for (uint8_t i = 0; i < NUM_BTNS; i++) {
                if (rising & (static_cast<uint16_t>(1u) << i)) {
                    toggleLed(static_cast<uint8_t>(i + 1));
                    Serial.printf("BTN%u pressed -> toggle LED%u\n", i + 1,
                                  i + 1);
                }
            }
            applyLeds();
        }
    }
}

// =============================================================================
// Arduino Lifecycle
// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(100); // USB-CDC Stabilisierung

    // --- LEDs (74HC595) ---
    pinMode(LED_DATA_PIN, OUTPUT);
    pinMode(LED_CLOCK_PIN, OUTPUT);
    pinMode(LED_LATCH_PIN, OUTPUT);
    digitalWrite(LED_DATA_PIN, LOW);
    digitalWrite(LED_CLOCK_PIN, LOW);
    digitalWrite(LED_LATCH_PIN, HIGH); // Latch inaktiv (LOW→HIGH übernimmt)

    // --- Buttons (CD4021) ---
    pinMode(BTN_DATA_PIN, INPUT);
    pinMode(BTN_CLOCK_PIN, OUTPUT);
    pinMode(BTN_LOAD_PIN, OUTPUT);
    digitalWrite(BTN_CLOCK_PIN, LOW);
    digitalWrite(BTN_LOAD_PIN, LOW); // Shift-Modus (nicht Load)

    // Start: alle LEDs aus
    g_ledState = 0;
    applyLeds();

    // Start: Button-Zustand initialisieren (keine Boot-Toggles)
    const uint32_t now = millis();
    g_btnRaw = readButtonsPressedMask();
    g_btnStable = g_btnRaw;
    g_btnChangedAt = now;

    Serial.println("=====================================");
    Serial.println("74HC595 + CD4021BE Hardwaretest v1.1");
    Serial.println("BTN1..BTN10 toggles LED1..LED10");
    Serial.println("=====================================");
}

void loop() {
    // Poll-Rate begrenzen (reduziert CPU-Last, stabiler für Debounce/Timing)
    static uint32_t lastPoll = 0;
    const uint32_t now = millis();

    if ((now - lastPoll) >= POLL_MS) {
        lastPoll = now;
        buttonsTask(now);
    }
}
