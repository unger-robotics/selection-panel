#include "config.h"
#include <SPI.h>

// =============================================================================
// SELECTION PANEL: Button→LED (1:1) mit One-Hot-Logik
// =============================================================================
// Jeder Taster steuert seine zugehörige LED. Bei mehreren gedrückten Tastern
// leuchtet nur die LED des zuletzt gedrückten Tasters (One-Hot).
// =============================================================================

// SPI-Konfigurationen für beide IC-Typen
static const SPISettings spiButtons(SPI_4021_HZ, MSBFIRST, SPI_4021_MODE);
static const SPISettings spiLEDs(SPI_595_HZ, MSBFIRST, SPI_595_MODE);

// -----------------------------------------------------------------------------
// Zustandsvariablen
// -----------------------------------------------------------------------------
// Taster (Active-Low: 0xFF = alle losgelassen)
static uint8_t btnRaw[BTN_BYTES];
static uint8_t btnRawPrev[BTN_BYTES];
static uint8_t btnDebounced[BTN_BYTES];
static uint8_t btnDebouncedPrev[BTN_BYTES];
static uint32_t btnLastChange[BTN_COUNT];

// LEDs (Active-High: 0x00 = alle aus)
static uint8_t ledState[LED_BYTES];
static uint8_t ledStatePrev[LED_BYTES];

// One-Hot-Auswahl
static uint8_t activeId = 0;
static uint8_t activeIdPrev = 0;

// =============================================================================
// Bit-Adressierung
// =============================================================================
// CD4021B und 74HC595 haben unterschiedliche Bit-Reihenfolgen:
// - CD4021B: MSB-first (Bit7=Eingang1, Bit0=Eingang8)
// - 74HC595: LSB-first (Bit0=Ausgang1, Bit7=Ausgang8)

// --- Taster (CD4021B): Bit7=T1, Bit0=T8, Active-Low ---
static inline bool btnIsPressed(const uint8_t *state, uint8_t id) {
    const uint8_t byte = (id - 1) / 8;
    const uint8_t bit = 7 - ((id - 1) % 8); // MSB-first
    return !(state[byte] & (1u << bit));    // Active-Low invertieren
}

static inline void btnSetBit(uint8_t *state, uint8_t id, bool pressed) {
    const uint8_t byte = (id - 1) / 8;
    const uint8_t bit = 7 - ((id - 1) % 8);
    if (pressed) {
        state[byte] &= ~(1u << bit); // Pressed → 0
    } else {
        state[byte] |= (1u << bit); // Released → 1
    }
}

// --- LEDs (74HC595): Bit0=LED1, Bit7=LED8, Active-High ---
static inline void ledSet(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT)
        return;
    const uint8_t byte = (id - 1) / 8;
    const uint8_t bit = (id - 1) % 8; // LSB-first
    if (on) {
        ledState[byte] |= (1u << bit);
    } else {
        ledState[byte] &= ~(1u << bit);
    }
}

static inline void ledClearAll() { memset(ledState, 0x00, LED_BYTES); }

// =============================================================================
// CD4021B einlesen (Hardware-SPI)
// =============================================================================
// Problem: Q8 liegt sofort nach Parallel-Load an, BEVOR der erste Clock kommt.
// Lösung: Erstes Bit per digitalRead() sichern, dann SPI-Daten korrigieren.

static void readButtons() {
    // 1. Parallel Load: Taster-Zustände ins Register übernehmen
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(5);

    // 2. Umschalten auf Shift-Mode
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(2);

    // 3. Erstes Bit sichern (Q8 liegt bereits stabil an MISO)
    const uint8_t firstBit = digitalRead(PIN_BTN_MISO) ? 1u : 0u;

    // 4. Restliche Bits per SPI einlesen
    uint8_t rx[BTN_BYTES];
    SPI.beginTransaction(spiButtons);
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        rx[i] = SPI.transfer(0x00);
    }
    SPI.endTransaction();

    // 5. Bit-Korrektur: Alle Bytes um 1 nach rechts, Carry durchreichen
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        const uint8_t carryIn =
            (i == 0)
                ? (firstBit << 7)            // Erstes Byte: gesichertes Bit
                : ((rx[i - 1] & 0x01) << 7); // Folgebytes: LSB des Vorgängers
        btnRaw[i] = (rx[i] >> 1) | carryIn;
    }
}

// =============================================================================
// 74HC595 beschreiben (Hardware-SPI)
// =============================================================================

static void latchOutputs() {
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_LED_RCK, LOW);
}

static void writeLEDs() {
    // Ghost-LEDs ausblenden
    ledState[LED_BYTES - 1] &= LED_LAST_MASK;

    SPI.beginTransaction(spiLEDs);
    // Rückwärts senden: Letztes Byte zuerst (rutscht durch die Kette)
    for (int i = LED_BYTES - 1; i >= 0; --i) {
        SPI.transfer(ledState[i]);
    }
    SPI.endTransaction();

    latchOutputs();
}

static void setBrightness(uint8_t percent) {
    if (!USE_OE_PWM)
        return;
    // OE ist active-low: 100% Helligkeit = 0% PWM-Duty
    const uint32_t maxDuty = (1u << LEDC_RESOLUTION) - 1;
    const uint32_t duty = maxDuty - (percent * maxDuty / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

// =============================================================================
// Debounce
// =============================================================================

static void debounceButtons() {
    if (!USE_DEBOUNCE) {
        memcpy(btnDebounced, btnRaw, BTN_BYTES);
        return;
    }

    const uint32_t now = millis();

    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool rawPressed = btnIsPressed(btnRaw, id);
        const bool prevPressed = btnIsPressed(btnRawPrev, id);
        const bool debPressed = btnIsPressed(btnDebounced, id);

        // Timer bei jeder Rohwert-Änderung zurücksetzen
        if (rawPressed != prevPressed) {
            btnLastChange[id - 1] = now;
        }

        // Übernahme nur nach stabiler Phase
        if ((now - btnLastChange[id - 1] >= DEBOUNCE_MS) &&
            (rawPressed != debPressed)) {
            btnSetBit(btnDebounced, id, rawPressed);
        }
    }

    memcpy(btnRawPrev, btnRaw, BTN_BYTES);
}

// =============================================================================
// One-Hot-Logik
// =============================================================================
// Bei mehreren gedrückten Tastern gewinnt der zuletzt gedrückte ("last press
// wins"). Wird der aktive Taster losgelassen, fällt die Auswahl auf den
// kleinsten noch gedrückten Taster zurück, oder auf 0 wenn keiner mehr gedrückt
// ist.

static void updateActiveId() {
    // Neuen Tastendruck erkennen (Flanke: nicht gedrückt → gedrückt)
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool wasPressed = btnIsPressed(btnDebouncedPrev, id);
        const bool isPressed = btnIsPressed(btnDebounced, id);
        if (!wasPressed && isPressed) {
            activeId = id; // Letzter Druck gewinnt
        }
    }

    // Fallback wenn aktiver Taster losgelassen wurde
    if (activeId != 0 && !btnIsPressed(btnDebounced, activeId)) {
        activeId = 0;
        for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
            if (btnIsPressed(btnDebounced, id)) {
                activeId = id;
                break; // Kleinste ID als Fallback
            }
        }
    }
}

// =============================================================================
// Debug-Ausgabe
// =============================================================================

static void printBinary(uint8_t value) {
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 1u);
    }
    Serial.printf(" (0x%02X)", value);
}

static void printState() {
    Serial.println("---");

    // Button-Zustände
    Serial.print("BTN RAW: ");
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (i)
            Serial.print(" | ");
        Serial.printf("IC%d: ", i);
        printBinary(btnRaw[i]);
    }
    Serial.println();

    Serial.print("BTN DEB: ");
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (i)
            Serial.print(" | ");
        Serial.printf("IC%d: ", i);
        printBinary(btnDebounced[i]);
    }
    Serial.println();

    // One-Hot-Auswahl
    Serial.printf("Active: Button=%d → LED=%d\n", activeId, activeId);

    // LED-Zustand
    Serial.print("LED:     ");
    for (size_t i = 0; i < LED_BYTES; ++i) {
        if (i)
            Serial.print(" | ");
        Serial.printf("IC%d: ", i);
        printBinary(ledState[i]);
    }
    Serial.println();

    // Gedrückte Taster als Liste
    Serial.print("Pressed: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (btnIsPressed(btnDebounced, id)) {
            if (any)
                Serial.print(' ');
            Serial.print(id);
            any = true;
        }
    }
    if (!any)
        Serial.print('-');
    Serial.println();
}

// =============================================================================
// Hilfsfunktionen
// =============================================================================

static inline bool hasButtonChange() {
    return memcmp(btnDebounced, btnDebouncedPrev, BTN_BYTES) != 0;
}

static inline bool hasLedChange() {
    return memcmp(ledState, ledStatePrev, LED_BYTES) != 0;
}

static inline bool anyButtonPressed() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnDebounced[i] != 0xFF)
            return true;
    }
    return false;
}

// =============================================================================
// Arduino-Hauptprogramm
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000); // PlatformIO Monitor braucht Zeit zum Verbinden

    // Button-Pins
    pinMode(PIN_BTN_PS, OUTPUT);
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
    digitalWrite(PIN_BTN_PS, LOW);

    // LED-Pins
    pinMode(PIN_LED_RCK, OUTPUT);
    digitalWrite(PIN_LED_RCK, LOW);

    // Hardware-SPI
    SPI.begin(PIN_SCK, PIN_BTN_MISO, PIN_LED_MOSI, -1);

    // PWM für Helligkeit
    if (USE_OE_PWM) {
        ledcSetup(LEDC_CHANNEL, PWM_FREQ, LEDC_RESOLUTION);
        ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
        setBrightness(PWM_DUTY);
    } else {
        pinMode(PIN_LED_OE, OUTPUT);
        digitalWrite(PIN_LED_OE, LOW); // Dauerhaft aktiv
    }

    // Definierte Ausgangszustände
    memset(btnRaw, 0xFF, BTN_BYTES);
    memset(btnRawPrev, 0xFF, BTN_BYTES);
    memset(btnDebounced, 0xFF, BTN_BYTES);
    memset(btnDebouncedPrev, 0xFF, BTN_BYTES);
    memset(btnLastChange, 0, sizeof(btnLastChange));

    memset(ledState, 0x00, LED_BYTES);
    memset(ledStatePrev, 0xFF, LED_BYTES); // Unterschiedlich → erster Write

    Serial.println();
    Serial.println("========================================");
    Serial.println("Selection Panel: Button→LED (One-Hot)");
    Serial.println("========================================");
    Serial.println("CD4021B: Bit7=T1, Bit0=T8 (Active-Low)");
    Serial.println("74HC595: Bit0=L1, Bit7=L8 (Active-High)");
    Serial.println("---");

    writeLEDs(); // Alle LEDs aus
}

void loop() {
    readButtons();
    debounceButtons();
    updateActiveId();

    // LED-Zustand aktualisieren
    ledClearAll();
    if (activeId != 0) {
        ledSet(activeId, true);
    }

    // LED nur bei Änderung aktualisieren (SPI-Traffic minimieren)
    if (hasLedChange()) {
        writeLEDs();
        memcpy(ledStatePrev, ledState, LED_BYTES);
    }

    // Debug-Ausgabe bei Änderung (keine leeren Blöcke)
    const bool activeChanged = (activeId != activeIdPrev);
    if ((hasButtonChange() || activeChanged) &&
        (anyButtonPressed() || activeChanged)) {
        printState();
    }

    memcpy(btnDebouncedPrev, btnDebounced, BTN_BYTES);
    activeIdPrev = activeId;

    delay(LOOP_DELAY_MS);
}
