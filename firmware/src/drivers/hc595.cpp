#include "drivers/hc595.h"

// =============================================================================
// 74HC595 Implementation
// =============================================================================

void Hc595::init() {
    // Latch-Pin (RCK/STCP): Steigende Flanke übernimmt Daten
    pinMode(PIN_LED_RCK, OUTPUT);
    digitalWrite(PIN_LED_RCK, LOW);

    // Output Enable (OE): Active-Low
    if (USE_OE_PWM) {
        // PWM für stufenlose Helligkeitsregelung
        ledcSetup(LEDC_CHANNEL, PWM_FREQ_HZ, LEDC_RESOLUTION);
        ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
        setBrightness(PWM_DUTY_PERCENT);
    } else {
        // Ohne PWM: OE dauerhaft aktiv (LOW)
        pinMode(PIN_LED_OE, OUTPUT);
        digitalWrite(PIN_LED_OE, LOW);
    }
}

void Hc595::setBrightness(uint8_t percent) {
    if (!USE_OE_PWM) return;

    // OE ist active-low: 100% Helligkeit = 0% PWM-Duty
    // percent=100 → duty=0   → OE dauerhaft LOW → volle Helligkeit
    // percent=0   → duty=255 → OE dauerhaft HIGH → aus
    const uint32_t maxDuty = (1u << LEDC_RESOLUTION) - 1;
    const uint32_t duty = maxDuty - (percent * maxDuty / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

void Hc595::maskUnused(uint8_t* state) {
    // Bei 10 LEDs: Byte 1 nutzt nur Bit 0-1 (LED 9-10)
    // Bits 2-7 auf 0 setzen, sonst könnten "Ghost-LEDs" leuchten
    if (LED_COUNT % 8 == 0) return;

    const uint8_t usedBits = LED_COUNT % 8;
    const uint8_t mask = static_cast<uint8_t>((1u << usedBits) - 1);
    state[LED_BYTES - 1] &= mask;
}

void Hc595::latch() {
    // Steigende Flanke an RCK übernimmt Schieberegister → Ausgänge
    // Alle ICs in der Kette latchen synchron
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(1);  // Setup-Zeit (Datenblatt: 20 ns)
    digitalWrite(PIN_LED_RCK, LOW);
}

void Hc595::write(SpiBus& bus, uint8_t* state) {
    maskUnused(state);

    {
        SpiGuard guard(bus, spi_);

        // Daisy-Chain: Letztes Byte zuerst senden
        // Es "rutscht durch" alle ICs und landet im letzten (IC1 = LED 9-10)
        // Das zuletzt gesendete Byte bleibt im ersten IC (IC0 = LED 1-8)
        for (int i = LED_BYTES - 1; i >= 0; --i) {
            SPI.transfer(state[i]);
        }
    }

    latch();
}
