#include "config.h"
#include <SPI.h>

// =============================================================================
// TASTER-TEST FIRMWARE: CD4021B Daisy-Chain über Hardware-SPI
// =============================================================================

static SPISettings spiSettings(SPI_HZ, MSBFIRST, SPI_MODE_BTN);

// Taster-Zustände (Active-Low: 0xFF = alle losgelassen)
static uint8_t btnRaw[BTN_BYTES];
static uint8_t btnRawPrev[BTN_BYTES];
static uint8_t btnDebounced[BTN_BYTES];
static uint8_t btnDebouncedPrev[BTN_BYTES];
static uint32_t btnLastChange[BTN_COUNT];

// =============================================================================
// Bit-Adressierung
// =============================================================================
// CD4021B liefert: Bit7 = Eingang P1 (Taster 1), Bit0 = Eingang P8 (Taster 8)
// Diese Zuordnung ist durch die Hardware vorgegeben (P1 → Q8, P8 → Q1).
// Active-Low: Gedrückt = 0, Losgelassen = 1 (Pull-Up an Eingängen).

struct BitAddress {
    uint8_t byte;
    uint8_t bit;
};

static inline BitAddress bitAddr(uint8_t id) {
    return {.byte = static_cast<uint8_t>((id - 1) / 8),
            .bit = static_cast<uint8_t>(7 - ((id - 1) % 8))};
}

static inline bool isPressed(const uint8_t *state, uint8_t id) {
    const auto addr = bitAddr(id);
    return !(state[addr.byte] & (1u << addr.bit)); // Active-Low invertieren
}

static inline void setBit(uint8_t *state, uint8_t id, bool pressed) {
    const auto addr = bitAddr(id);
    if (pressed) {
        state[addr.byte] &= ~(1u << addr.bit); // Pressed → 0
    } else {
        state[addr.byte] |= (1u << addr.bit); // Released → 1
    }
}

// =============================================================================
// CD4021B einlesen
// =============================================================================
// Problem: Der CD4021B stellt Q8 sofort nach Parallel-Load bereit, BEVOR der
// erste Clock-Impuls kommt. SPI samplet aber erst NACH der ersten Clock-Flanke.
// Ohne Korrektur verlieren wir Bit 1 und lesen stattdessen Bit 2-17.
//
// Lösung: Erstes Bit per digitalRead() vor SPI-Transfer sichern, dann die
// 16 SPI-Bits um 1 nach rechts schieben und das gesicherte Bit als MSB
// einsetzen.

static void readButtons() {
    // 1. Parallel Load: Taster-Zustände ins Schieberegister übernehmen
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(
        5); // t_PW(PL) min = 40 ns, Sicherheitsmarge für Breadboard

    // 2. Umschalten auf Shift-Mode
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(2); // Stabilisierungszeit für Q8

    // 3. Erstes Bit sichern (Q8 liegt bereits stabil an MISO)
    const uint8_t firstBit = digitalRead(PIN_BTN_MISO) ? 1u : 0u;

    // 4. Restliche 16 Bits per SPI einlesen
    SPI.beginTransaction(spiSettings);
    const uint8_t rx0 = SPI.transfer(0x00); // Bits 2-9 (nach Shift)
    const uint8_t rx1 = SPI.transfer(0x00); // Bits 10-17 (nach Shift)
    SPI.endTransaction();

    // 5. Korrektur: Gesichertes Bit als MSB einsetzen
    const uint16_t shifted = (static_cast<uint16_t>(rx0) << 8) | rx1;
    const uint16_t corrected =
        (static_cast<uint16_t>(firstBit) << 15) | (shifted >> 1);

    btnRaw[0] = static_cast<uint8_t>(corrected >> 8);
    btnRaw[1] = static_cast<uint8_t>(corrected & 0xFF);
}

// =============================================================================
// Debounce
// =============================================================================
// Zeitbasiertes Debouncing: Ein Zustandswechsel wird erst übernommen, wenn der
// Rohwert für DEBOUNCE_MS stabil geblieben ist. Jeder Taster hat seinen eigenen
// Timer, sodass schnelle Tastenfolgen korrekt erfasst werden.

static void debounceButtons() {
    if (!USE_DEBOUNCE) {
        memcpy(btnDebounced, btnRaw, BTN_BYTES);
        return;
    }

    const uint32_t now = millis();

    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool rawPressed = isPressed(btnRaw, id);
        const bool prevPressed = isPressed(btnRawPrev, id);
        const bool debPressed = isPressed(btnDebounced, id);

        // Timer zurücksetzen bei jeder Rohwert-Änderung
        if (rawPressed != prevPressed) {
            btnLastChange[id - 1] = now;
        }

        // Übernahme nur nach stabiler Phase UND bei tatsächlicher Änderung
        if ((now - btnLastChange[id - 1] >= DEBOUNCE_MS) &&
            (rawPressed != debPressed)) {
            setBit(btnDebounced, id, rawPressed);
        }
    }

    memcpy(btnRawPrev, btnRaw, BTN_BYTES);
}

// =============================================================================
// LED-Ausgänge deaktivieren
// =============================================================================
// Die 74HC595 teilen sich den SPI-Bus. Ohne definierten Zustand können
// zufällige Daten in den Schieberegistern stehen → LEDs flackern.

static void disableLEDs() {
    // OE auf HIGH = Ausgänge hochohmig (LEDs aus, unabhängig vom
    // Registerinhalt)
    pinMode(PIN_LED_OE, OUTPUT);
    digitalWrite(PIN_LED_OE, HIGH);

    // Latch-Pin vorbereiten
    pinMode(PIN_LED_RCK, OUTPUT);
    digitalWrite(PIN_LED_RCK, LOW);

    // Schieberegister mit 0x00 füllen (für sauberen Zustand)
    SPI.beginTransaction(spiSettings);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.endTransaction();

    // Latch: Nullen an Ausgänge übernehmen
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_LED_RCK, LOW);
}

// =============================================================================
// Debug-Ausgabe
// =============================================================================

static bool hasChange() {
    return memcmp(btnDebounced, btnDebouncedPrev, BTN_BYTES) != 0;
}

static bool anyPressed() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnDebounced[i] != 0xFF)
            return true; // Active-Low: != 0xFF → gedrückt
    }
    return false;
}

static void printBinary(const uint8_t *arr, size_t bytes) {
    for (size_t i = 0; i < bytes; ++i) {
        if (i)
            Serial.print(' ');
        for (int8_t bit = 7; bit >= 0; --bit) {
            Serial.print((arr[i] >> bit) & 1u);
        }
    }
}

static void printState() {
    Serial.print("RAW: ");
    printBinary(btnRaw, BTN_BYTES);
    Serial.print(" | DEB: ");
    printBinary(btnDebounced, BTN_BYTES);

    Serial.print(" → Pressed: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (isPressed(btnDebounced, id)) {
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
// Arduino-Hauptprogramm
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000); // PlatformIO Monitor braucht Zeit zum Verbinden

    pinMode(PIN_BTN_PS, OUTPUT);
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
    digitalWrite(PIN_BTN_PS, LOW);

    // Hardware-SPI initialisieren
    SPI.begin(PIN_SCK, PIN_BTN_MISO, PIN_SPI_MOSI, -1);

    // LEDs definiert ausschalten (shared SPI-Bus)
    disableLEDs();

    // Definierter Ausgangszustand: Alle Taster losgelassen (0xFF)
    memset(btnRaw, 0xFF, BTN_BYTES);
    memset(btnRawPrev, 0xFF, BTN_BYTES);
    memset(btnDebounced, 0xFF, BTN_BYTES);
    memset(btnDebouncedPrev, 0xFF, BTN_BYTES);
    memset(btnLastChange, 0, sizeof(btnLastChange));

    Serial.println();
    Serial.println("========================================");
    Serial.println("Taster-Test: CD4021B via Hardware-SPI");
    Serial.println("========================================");
    Serial.println("Bit-Zuordnung: Bit7=T1 ... Bit0=T8");
    Serial.println("Active-Low: 0=pressed, 1=released");
    Serial.println("---");
}

void loop() {
    readButtons();
    debounceButtons();

    // Ausgabe nur bei Änderung UND wenn mindestens ein Taster gedrückt
    if (hasChange() && anyPressed()) {
        printState();
    }

    memcpy(btnDebouncedPrev, btnDebounced, BTN_BYTES);
    delay(LOOP_DELAY_MS);
}
