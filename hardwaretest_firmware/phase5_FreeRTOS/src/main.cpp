#include "config.h"
#include <Arduino.h>
#include <SPI.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// =============================================================================
// SELECTION PANEL: FreeRTOS-basierte Firmware
// =============================================================================
//
// Warum FreeRTOS statt einfacher loop()?
// 1. Garantiertes Timing: vTaskDelayUntil() kompensiert Ausführungszeit
// 2. Entkopplung: Serial-Ausgabe blockiert nicht das IO-Timing
// 3. Skalierbarkeit: Weitere Tasks (Audio, Web) ohne Umbau hinzufügbar
//
// Warum Queue für Logging?
// USB-CDC (Serial) ist langsam und kann blockieren. Ohne Queue würde
// Serial.print() den IO-Task aufhalten → Debounce-Timing gestört.
// Mit Queue: IO schreibt Event in Queue (nicht-blockierend), Serial-Task
// liest und gibt aus wenn CPU frei ist.
//
// =============================================================================

// SPI-Konfigurationen (unterschiedliche Modi für CD4021B und 74HC595)
static SPISettings spiButtons(SPI_HZ_BTN, MSBFIRST, SPI_MODE_BTN);
static SPISettings spiLEDs(SPI_HZ_LED, MSBFIRST, SPI_MODE_LED);

// =============================================================================
// Datentypen
// =============================================================================

// Log-Event: Snapshot des gesamten Systemzustands für die Queue
struct LogEvent {
    uint32_t timestamp;
    uint8_t btnRaw[BTN_BYTES];
    uint8_t btnDebounced[BTN_BYTES];
    uint8_t ledState[LED_BYTES];
    uint8_t activeId;
    bool rawChanged;
    bool debChanged;
    bool activeChanged;
};

// =============================================================================
// Globale Zustandsvariablen
// =============================================================================
// static: Nur in dieser Datei sichtbar (keine externen Abhängigkeiten)

static QueueHandle_t logQueue = nullptr;

// Taster-Zustand (Active-Low: 0xFF = alle losgelassen)
static uint8_t btnRaw[BTN_BYTES];
static uint8_t btnRawPrev[BTN_BYTES];
static uint8_t btnDebounced[BTN_BYTES];
static uint8_t btnDebouncedPrev[BTN_BYTES];
static uint32_t btnLastChange[BTN_COUNT];

// LED-Zustand (Active-High: 0x00 = alle aus)
static uint8_t ledState[LED_BYTES];

// Aktuelle Auswahl (0 = keine, 1-10 = Taster-ID)
static uint8_t activeId = 0;

// =============================================================================
// Bit-Adressierung: Taster (CD4021B)
// =============================================================================
// CD4021B liefert MSB-first: Bit7 = P1 (Taster 1), Bit0 = P8 (Taster 8)
// Active-Low: 0 = gedrückt, 1 = losgelassen

static inline uint8_t btnByteIndex(uint8_t id) { return (id - 1) / 8; }

static inline uint8_t btnBitIndex(uint8_t id) {
    return 7 - ((id - 1) % 8); // MSB-first: ID 1 → Bit 7
}

static inline bool btnIsPressed(const uint8_t *state, uint8_t id) {
    const uint8_t byte = btnByteIndex(id);
    const uint8_t bit = btnBitIndex(id);
    return !(state[byte] & (1u << bit)); // Active-Low invertieren
}

static inline void btnSetBit(uint8_t *state, uint8_t id, bool pressed) {
    const uint8_t byte = btnByteIndex(id);
    const uint8_t bit = btnBitIndex(id);
    if (pressed) {
        state[byte] &= ~(1u << bit); // Pressed → 0
    } else {
        state[byte] |= (1u << bit); // Released → 1
    }
}

// =============================================================================
// Bit-Adressierung: LEDs (74HC595)
// =============================================================================
// 74HC595 ist LSB-first: Bit0 = QA (LED 1), Bit7 = QH (LED 8)
// Active-High: 1 = an, 0 = aus

static inline uint8_t ledByteIndex(uint8_t id) { return (id - 1) / 8; }

static inline uint8_t ledBitIndex(uint8_t id) {
    return (id - 1) % 8; // LSB-first: ID 1 → Bit 0
}

static inline void ledClearAll() { memset(ledState, 0x00, LED_BYTES); }

static inline void ledSet(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT)
        return;
    const uint8_t byte = ledByteIndex(id);
    const uint8_t bit = ledBitIndex(id);
    if (on) {
        ledState[byte] |= (1u << bit);
    } else {
        ledState[byte] &= ~(1u << bit);
    }
}

static inline void ledMaskUnused() {
    // Unbenutzte Bits im letzten Byte auf 0 setzen (Ghost-LEDs verhindern)
    if (LED_COUNT % 8 == 0)
        return;
    const uint8_t usedBits = LED_COUNT % 8;
    const uint8_t mask = static_cast<uint8_t>((1u << usedBits) - 1u);
    ledState[LED_BYTES - 1] &= mask;
}

// =============================================================================
// Hardware-Zugriff: 74HC595 (LED-Ausgabe)
// =============================================================================

static inline void latchOutputs() {
    // Steigende Flanke übernimmt Schieberegister → Ausgangsregister
    digitalWrite(PIN_LED_RCK, HIGH);
    delayMicroseconds(1);
    digitalWrite(PIN_LED_RCK, LOW);
}

static void setBrightness(uint8_t percent) {
    // OE ist active-low: 100% Helligkeit = 0% PWM-Duty
    const uint32_t maxDuty = (1u << LEDC_RESOLUTION) - 1;
    const uint32_t duty = maxDuty - (percent * maxDuty / 100);
    ledcWrite(LEDC_CHANNEL, duty);
}

static void writeLEDs() {
    ledMaskUnused();

    SPI.beginTransaction(spiLEDs);
    // Rückwärts senden: Letztes Byte zuerst (rutscht durch die Kette)
    for (int i = LED_BYTES - 1; i >= 0; --i) {
        SPI.transfer(ledState[i]);
    }
    SPI.endTransaction();

    latchOutputs();
}

// =============================================================================
// Hardware-Zugriff: CD4021B (Taster-Eingabe)
// =============================================================================
// Das "First-Bit-Problem": Q8 liegt sofort nach Parallel-Load an, BEVOR der
// erste Clock kommt. SPI samplet aber erst NACH der ersten Flanke.
// Ohne Korrektur: Bit 1 geht verloren, wir lesen Bit 2-17 statt 1-16.

static void readButtons(uint8_t *output) {
    // 1. Parallel Load: Taster-Zustände ins Register "fotografieren"
    digitalWrite(PIN_BTN_PS, HIGH);
    delayMicroseconds(5); // Hold-Zeit für stabiles Einlesen

    // 2. Umschalten auf Shift-Mode
    digitalWrite(PIN_BTN_PS, LOW);
    delayMicroseconds(1);

    // 3. KRITISCH: Erstes Bit vor SPI sichern (liegt bereits an Q8)
    const uint8_t firstBit = digitalRead(PIN_BTN_MISO) ? 1u : 0u;

    // 4. Restliche Bits per SPI einlesen
    uint8_t rx[BTN_BYTES];
    SPI.beginTransaction(spiButtons);
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        rx[i] = SPI.transfer(0x00);
    }
    SPI.endTransaction();

    // 5. Bit-Korrektur: Alle Bytes um 1 nach rechts, Carry durchreichen
    //    Byte 0: [firstBit | rx[0] >> 1]
    //    Byte n: [rx[n-1] bit0 | rx[n] >> 1]
    output[0] = static_cast<uint8_t>((firstBit << 7) | (rx[0] >> 1));
    for (size_t i = 1; i < BTN_BYTES; ++i) {
        output[i] =
            static_cast<uint8_t>(((rx[i - 1] & 0x01) << 7) | (rx[i] >> 1));
    }
}

// =============================================================================
// Debounce-Logik
// =============================================================================
// Zeitbasiert: Ein Zustandswechsel wird erst übernommen, wenn der Rohwert
// für DEBOUNCE_MS stabil geblieben ist.

static bool debounceUpdate(uint32_t now) {
    bool anyChanged = false;

    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool rawPressed = btnIsPressed(btnRaw, id);
        const bool prevPressed = btnIsPressed(btnRawPrev, id);
        const bool debPressed = btnIsPressed(btnDebounced, id);

        // Timer bei jeder Rohwert-Änderung zurücksetzen
        if (rawPressed != prevPressed) {
            btnLastChange[id - 1] = now;
        }

        // Übernahme nur nach stabiler Phase UND bei tatsächlicher Änderung
        if ((now - btnLastChange[id - 1] >= DEBOUNCE_MS) &&
            (rawPressed != debPressed)) {
            btnSetBit(btnDebounced, id, rawPressed);
            anyChanged = true;
        }
    }

    memcpy(btnRawPrev, btnRaw, BTN_BYTES);
    return anyChanged;
}

// =============================================================================
// Selection-Logik (One-Hot)
// =============================================================================

static inline bool anyButtonPressed() {
    for (size_t i = 0; i < BTN_BYTES; ++i) {
        if (btnDebounced[i] != 0xFF)
            return true; // Active-Low
    }
    return false;
}

// Erkennt Flanken (nicht gedrückt → gedrückt) und wählt den letzten Druck
static uint8_t selectFromEdges() {
    uint8_t newActive = activeId;

    // Flanken-Erkennung: Wer wurde gerade gedrückt?
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        const bool wasPressed = btnIsPressed(btnDebouncedPrev, id);
        const bool isPressed = btnIsPressed(btnDebounced, id);

        if (isPressed && !wasPressed) {
            newActive = id; // "Last press wins"
        }
    }

    // Ohne Latch: Auswahl erlischt wenn nichts gedrückt
    if (!LATCH_SELECTION && !anyButtonPressed()) {
        newActive = 0;
    }

    return newActive;
}

static void applyOneHotLED() {
    ledClearAll();
    if (activeId >= 1 && activeId <= LED_COUNT) {
        ledSet(activeId, true);
    }
}

// =============================================================================
// Debug-Ausgabe (nur im Serial-Task verwendet)
// =============================================================================

static void printBinary(uint8_t value) {
    for (int8_t bit = 7; bit >= 0; --bit) {
        Serial.print((value >> bit) & 1u);
    }
}

static void printByteArray(const char *label, const uint8_t *arr,
                           size_t bytes) {
    Serial.print(label);
    for (size_t i = 0; i < bytes; ++i) {
        if (i)
            Serial.print(" | ");
        Serial.printf("IC%d: ", i);
        printBinary(arr[i]);
        Serial.printf(" (0x%02X)", arr[i]);
    }
    Serial.println();
}

static void printPressedList(const uint8_t *debounced) {
    Serial.print("Pressed IDs: ");
    bool any = false;
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        if (btnIsPressed(debounced, id)) {
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

static void printButtonsVerbose(const uint8_t *raw, const uint8_t *debounced) {
    Serial.println("Buttons per ID (RAW/DEB)  [pressed=1 | released=0]");
    for (uint8_t id = 1; id <= BTN_COUNT; ++id) {
        Serial.printf("  T%02d  IC%d b%d   RAW=%d  DEB=%d\n", id,
                      btnByteIndex(id), btnBitIndex(id),
                      btnIsPressed(raw, id) ? 1 : 0,
                      btnIsPressed(debounced, id) ? 1 : 0);
    }
}

static void printLEDsVerbose(const uint8_t *led) {
    Serial.println("LEDs per ID (state)       [on=1 | off=0]");
    for (uint8_t id = 1; id <= LED_COUNT; ++id) {
        const bool on = (led[ledByteIndex(id)] >> ledBitIndex(id)) & 1u;
        Serial.printf("  L%02d  IC%d b%d   %d\n", id, ledByteIndex(id),
                      ledBitIndex(id), on ? 1 : 0);
    }
}

// =============================================================================
// FreeRTOS Task: Serial-Ausgabe
// =============================================================================
// Läuft mit hoher Priorität, aber blockiert auf Queue.
// Gibt Log-Events aus sobald verfügbar, ohne IO zu stören.

static void taskSerial(void *) {
    LogEvent event;

    for (;;) {
        // Blockiert bis Event in Queue (portMAX_DELAY = unendlich warten)
        if (xQueueReceive(logQueue, &event, portMAX_DELAY) == pdTRUE) {
            Serial.println("---");
            printByteArray("BTN RAW:    ", event.btnRaw, BTN_BYTES);
            printByteArray("BTN DEB:    ", event.btnDebounced, BTN_BYTES);
            Serial.printf("Active LED (One-Hot): %d\n", event.activeId);
            printByteArray("LED STATE:  ", event.ledState, LED_BYTES);
            printPressedList(event.btnDebounced);

            if (LOG_VERBOSE_PER_ID) {
                printButtonsVerbose(event.btnRaw, event.btnDebounced);
                printLEDsVerbose(event.ledState);
            }

            // Kurz abgeben damit andere Tasks laufen können
            vTaskDelay(1);
        }
    }
}

// =============================================================================
// FreeRTOS Task: IO-Verarbeitung
// =============================================================================
// Läuft mit niedriger Priorität, aber garantiertem Timing durch
// vTaskDelayUntil. Liest Taster, entprellt, wählt aus, schreibt LEDs.

static void taskIO(void *) {
    // Initialisierung der Zustandsvariablen
    memset(btnRaw, 0xFF, BTN_BYTES);
    memset(btnRawPrev, 0xFF, BTN_BYTES);
    memset(btnDebounced, 0xFF, BTN_BYTES);
    memset(btnDebouncedPrev, 0xFF, BTN_BYTES);
    memset(btnLastChange, 0, sizeof(btnLastChange));
    memset(ledState, 0x00, LED_BYTES);
    activeId = 0;

    // LEDs initial ausschalten
    applyOneHotLED();
    writeLEDs();

    // Für präzises Timing: Startzeit merken
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        // Warten bis nächste Periode (kompensiert Ausführungszeit)
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(IO_PERIOD_MS));

        // 1. Taster einlesen
        uint8_t newRaw[BTN_BYTES];
        readButtons(newRaw);
        memcpy(btnRaw, newRaw, BTN_BYTES);

        const bool rawChanged = (memcmp(btnRaw, btnRawPrev, BTN_BYTES) != 0);

        // 2. Entprellen
        const uint32_t now = millis();
        const bool debChanged = debounceUpdate(now);

        // 3. Auswahl aktualisieren
        const uint8_t prevActive = activeId;
        activeId = selectFromEdges();
        const bool activeChanged = (activeId != prevActive);

        // 4. Debounced-Prev für nächste Flanken-Erkennung
        memcpy(btnDebouncedPrev, btnDebounced, BTN_BYTES);

        // 5. LEDs aktualisieren
        applyOneHotLED();
        writeLEDs();

        // 6. Log-Event erstellen (nur bei relevanter Änderung)
        const bool shouldLog =
            debChanged || activeChanged || (LOG_ON_RAW_CHANGE && rawChanged);

        if (shouldLog) {
            LogEvent event = {};
            event.timestamp = now;
            memcpy(event.btnRaw, btnRaw, BTN_BYTES);
            memcpy(event.btnDebounced, btnDebounced, BTN_BYTES);
            memcpy(event.ledState, ledState, LED_BYTES);
            event.activeId = activeId;
            event.rawChanged = rawChanged;
            event.debChanged = debChanged;
            event.activeChanged = activeChanged;

            // Nicht-blockierend: Bei voller Queue wird Event verworfen
            // (IO-Timing bleibt sauber, Logging ist nicht kritisch)
            xQueueSend(logQueue, &event, 0);
        }
    }
}

// =============================================================================
// Arduino Entry Points
// =============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000); // USB-CDC braucht Zeit zum Initialisieren

    Serial.println();
    Serial.println("========================================");
    Serial.println("Selection Panel: FreeRTOS + One-Hot");
    Serial.println("========================================");
    Serial.printf("IO_PERIOD_MS:    %lu\n", IO_PERIOD_MS);
    Serial.printf("DEBOUNCE_MS:     %lu\n", DEBOUNCE_MS);
    Serial.printf("LATCH_SELECTION: %s\n", LATCH_SELECTION ? "true" : "false");
    Serial.println("---");

    // GPIO initialisieren
    pinMode(PIN_BTN_PS, OUTPUT);
    digitalWrite(PIN_BTN_PS, LOW);

    pinMode(PIN_LED_RCK, OUTPUT);
    digitalWrite(PIN_LED_RCK, LOW);

    // Hardware-SPI (gemeinsamer Bus)
    SPI.begin(PIN_SCK, PIN_BTN_MISO, PIN_LED_MOSI, -1);

    // PWM für LED-Helligkeit
    ledcSetup(LEDC_CHANNEL, PWM_FREQ_HZ, LEDC_RESOLUTION);
    ledcAttachPin(PIN_LED_OE, LEDC_CHANNEL);
    setBrightness(PWM_DUTY_PERCENT);

    // Log-Queue erstellen
    logQueue = xQueueCreate(LOG_QUEUE_LEN, sizeof(LogEvent));

    // Tasks starten (beide auf Core 1, Core 0 ist für WiFi/BLE reserviert)
    xTaskCreatePinnedToCore(taskSerial, "Serial", 8192, nullptr, PRIO_SERIAL,
                            nullptr, CORE_APP);
    xTaskCreatePinnedToCore(taskIO, "IO", 8192, nullptr, PRIO_IO, nullptr,
                            CORE_APP);
}

void loop() {
    // loop() bleibt leer – Tasks übernehmen die Arbeit
    // vTaskDelay(portMAX_DELAY) spart CPU-Zeit
    vTaskDelay(portMAX_DELAY);
}
