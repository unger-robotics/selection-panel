#include "config.h"
#include <SPI.h>

// =============================================================================
// LED-TEST FIRMWARE: Lauflicht über 74HC595-Kette
// =============================================================================

static const SPISettings spiSettings(SPI_HZ, MSBFIRST, SPI_MODE0);

// LED-Zustand als Bitfeld: Jedes Bit entspricht einer LED (ID 1 = Bit 0)
static uint8_t ledState[LED_BYTES] = {0};
static uint8_t currentLED = 1;

// =============================================================================
// Bit-Adressierung
// =============================================================================
// LED-IDs sind 1-basiert (menschenfreundlich), intern 0-basiert verarbeitet.
// ID 1-8 → Byte 0, ID 9-16 → Byte 1, usw.

static inline uint8_t byteIndex(uint8_t id) { return (id - 1) / 8; }
static inline uint8_t bitPosition(uint8_t id) { return (id - 1) % 8; }

// =============================================================================
// LED-Steuerung
// =============================================================================

static void clearAllLEDs() { memset(ledState, 0, LED_BYTES); }

static void setLED(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT)
        return;

    const uint8_t byte = byteIndex(id);
    const uint8_t bit = bitPosition(id);

    if (on) {
        ledState[byte] |= (1u << bit);
    } else {
        ledState[byte] &= ~(1u << bit);
    }
}

// =============================================================================
// Hardware-Ausgabe
// =============================================================================

static void latchOutputs() {
    // Steigende Flanke an RCK übernimmt Schieberegister → Ausgangsregister.
    // Alle 74HC595 in der Kette latchen synchron.
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(1); // t_su(RCK) = 20ns laut Datenblatt, Sicherheitsmarge
    digitalWrite(PIN_LED_RCK, LOW);
}

static void transferToShiftRegisters() {
    // Ghost-LEDs ausblenden: Bits 2-7 im letzten Byte auf 0 setzen,
    // sonst leuchten nicht vorhandene LEDs bei fehlerhafter Hardware.
    ledState[LED_BYTES - 1] &= LED_LAST_MASK;

    SPI.beginTransaction(spiSettings);

    // Rückwärts senden: Byte[1] zuerst, dann Byte[0].
    // Grund: Daten "rutschen durch" die Daisy-Chain.
    // Das zuerst gesendete Byte landet im letzten IC (IC1 = LED 9-16),
    // das zuletzt gesendete im ersten IC (IC0 = LED 1-8).
    for (int i = LED_BYTES - 1; i >= 0; --i) {
        SPI.transfer(ledState[i]);
    }

    SPI.endTransaction();
    latchOutputs();
}

static void setBrightness(uint8_t percent) {
    if (!USE_OE_PWM)
        return;

    // OE ist active-low: Duty invertieren.
    // percent=100 → duty=0 → OE dauerhaft LOW → volle Helligkeit
    // percent=0   → duty=255 → OE dauerhaft HIGH → aus
    const uint32_t maxDuty = (1u << LEDC_RESOLUTION) - 1;
    const uint32_t duty = maxDuty - (percent * maxDuty / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

// =============================================================================
// CD4021B-Vorbereitung
// =============================================================================

static void prepareButtonInputs() {
    // CD4021B teilt sich SCLK mit 74HC595.
    // Problem: Jeder LED-Takt schiebt auch das Button-Register weiter.
    // Lösung: PS (Parallel/Serial) auf HIGH halten = Serial-Mode.
    // Im Serial-Mode ignoriert CD4021B den Takt für Parallel-Load,
    // die Taster-Zustände bleiben stabil bis wir sie explizit einlesen.
    pinMode(PIN_BTN_PS, OUTPUT);
    digitalWrite(PIN_BTN_PS, HIGH);

    // MISO-Pin vorbereiten (wird erst beim Taster-Einlesen genutzt)
    pinMode(PIN_BTN_MISO, INPUT_PULLUP);
}

// =============================================================================
// Debug-Ausgabe
// =============================================================================

static void printState() {
    for (size_t ic = 0; ic < LED_BYTES; ++ic) {
        Serial.printf("IC%d: ", ic);

        // Binärdarstellung (MSB links)
        for (int8_t bit = 7; bit >= 0; --bit) {
            Serial.print((ledState[ic] >> bit) & 1);
        }

        Serial.printf(" (0x%02X)", ledState[ic]);
        if (ic + 1 < LED_BYTES)
            Serial.print(" | ");
    }
    Serial.printf(" → LED %d aktiv\n", currentLED);
}

// =============================================================================
// Arduino-Hauptprogramm
// =============================================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 1500) { /* Warten auf USB-Serial */
    }

    // Latch-Pin initialisieren (LOW = bereit für nächsten Transfer)
    pinMode(PIN_LED_RCK, OUTPUT);
    digitalWrite(PIN_LED_RCK, LOW);

    // Hardware-SPI: SCK, MISO, MOSI, SS (-1 = kein Hardware-SS)
    SPI.begin(PIN_SCK, PIN_BTN_MISO, PIN_LED_MOSI, -1);

    // PWM für Helligkeitsregelung
    if (USE_OE_PWM) {
        ledcSetup(LEDC_CHANNEL, PWM_FREQ, LEDC_RESOLUTION);
        ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
        setBrightness(PWM_DUTY);
    }

    prepareButtonInputs();

    // Definierter Ausgangszustand: Alle LEDs aus
    clearAllLEDs();
    transferToShiftRegisters();

    Serial.println("=== LED-Test gestartet ===");
}

void loop() {
    // Lauflicht: Eine LED nach der anderen
    clearAllLEDs();
    setLED(currentLED, true);
    transferToShiftRegisters();
    printState();

    // Nächste LED (zyklisch 1 → 10 → 1 → ...)
    currentLED = (currentLED >= LED_COUNT) ? 1 : currentLED + 1;
    delay(CHASE_DELAY_MS);
}
