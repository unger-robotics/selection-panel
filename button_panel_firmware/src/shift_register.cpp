/**
 * @file shift_register.cpp
 * @brief Thread-sichere Implementation fuer 74HC595 und CD4021BE
 * @version 2.3.1
 * @date 2025-01-01
 *
 * CHANGELOG v2.3.1:
 * - FIX: CD4021 shiftInData() - kein Clock nach letztem Bit (aus v1.1.1)
 * - FIX: Timing-Delays an Datenblatt angepasst (2µs Load, 1µs Clock)
 */

#include "shift_register.h"

// =============================================================================
// OutputShiftRegister (74HC595)
// =============================================================================

OutputShiftRegister::OutputShiftRegister(uint8_t dataPin, uint8_t clockPin,
                                         uint8_t latchPin, uint8_t numChips)
    : _dataPin(dataPin), _clockPin(clockPin), _latchPin(latchPin),
      _numChips(min(numChips, MAX_SHIFT_REGS)), _mutex(nullptr),
      _initialized(false) {
    memset(_buffer, 0, sizeof(_buffer));
}

OutputShiftRegister::~OutputShiftRegister() {
    if (_mutex != nullptr) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

bool OutputShiftRegister::begin() {
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        return false;
    }

    pinMode(_dataPin, OUTPUT);
    pinMode(_clockPin, OUTPUT);
    pinMode(_latchPin, OUTPUT);

    digitalWrite(_dataPin, LOW);
    digitalWrite(_clockPin, LOW);
    digitalWrite(_latchPin, LOW);

    clear();
    update();

    _initialized = true;
    return true;
}

void OutputShiftRegister::write(const uint8_t *data) {
    if (!takeMutex())
        return;

    for (uint8_t i = 0; i < _numChips; i++) {
        _buffer[i] = data[i];
    }
    shiftOutData();
    pulseLatch();

    giveMutex();
}

void OutputShiftRegister::setOutput(uint8_t index, bool state) {
    uint8_t maxBits = _numChips * 8;
    if (index >= maxBits)
        return;
    if (!takeMutex())
        return;

    uint8_t chip = index / 8;
    uint8_t bit = index % 8;

    if (state) {
        _buffer[chip] |= (1 << bit);
    } else {
        _buffer[chip] &= ~(1 << bit);
    }

    giveMutex();
}

bool OutputShiftRegister::getOutput(uint8_t index) const {
    uint8_t maxBits = _numChips * 8;
    if (index >= maxBits)
        return false;

    uint8_t chip = index / 8;
    uint8_t bit = index % 8;

    return (_buffer[chip] >> bit) & 0x01;
}

void OutputShiftRegister::clear() {
    if (!takeMutex())
        return;
    memset(_buffer, 0, _numChips);
    giveMutex();
}

void OutputShiftRegister::setAll() {
    if (!takeMutex())
        return;
    memset(_buffer, 0xFF, _numChips);
    giveMutex();
}

void OutputShiftRegister::setOnly(int8_t index) {
    if (!takeMutex())
        return;

    memset(_buffer, 0, _numChips);

    uint8_t maxBits = _numChips * 8;
    if (index >= 0 && index < maxBits) {
        uint8_t chip = index / 8;
        uint8_t bit = index % 8;
        _buffer[chip] |= (1 << bit);
    }

    shiftOutData();
    pulseLatch();

    giveMutex();
}

void OutputShiftRegister::update() {
    if (!takeMutex())
        return;
    shiftOutData();
    pulseLatch();
    giveMutex();
}

void OutputShiftRegister::shiftOutData() {
    // Daten von hinten nach vorne schieben (letzter Chip zuerst)
    for (int8_t chip = _numChips - 1; chip >= 0; chip--) {
        uint8_t data = _buffer[chip];

        // MSB first fuer korrekte Bit-Reihenfolge
        for (int8_t bit = 7; bit >= 0; bit--) {
            digitalWrite(_dataPin, (data >> bit) & 0x01);
            delayMicroseconds(SHIFT_DELAY_US);
            pulseClock();
        }
    }
}

void OutputShiftRegister::pulseClock() {
    digitalWrite(_clockPin, HIGH);
    delayMicroseconds(SHIFT_DELAY_US);
    digitalWrite(_clockPin, LOW);
    delayMicroseconds(SHIFT_DELAY_US);
}

void OutputShiftRegister::pulseLatch() {
    digitalWrite(_latchPin, HIGH);
    delayMicroseconds(SHIFT_DELAY_US);
    digitalWrite(_latchPin, LOW);
    delayMicroseconds(SHIFT_DELAY_US);
}

bool OutputShiftRegister::takeMutex(TickType_t timeout) {
    if (_mutex == nullptr)
        return false;
    return xSemaphoreTake(_mutex, timeout) == pdTRUE;
}

void OutputShiftRegister::giveMutex() {
    if (_mutex != nullptr) {
        xSemaphoreGive(_mutex);
    }
}

// =============================================================================
// InputShiftRegister (CD4021BE)
// =============================================================================

InputShiftRegister::InputShiftRegister(uint8_t dataPin, uint8_t clockPin,
                                       uint8_t loadPin, uint8_t numChips)
    : _dataPin(dataPin), _clockPin(clockPin), _loadPin(loadPin),
      _numChips(min(numChips, MAX_SHIFT_REGS)), _mutex(nullptr),
      _initialized(false) {
    // Pull-ups: alle HIGH wenn nicht gedrueckt
    memset(_buffer, 0xFF, sizeof(_buffer));
}

InputShiftRegister::~InputShiftRegister() {
    if (_mutex != nullptr) {
        vSemaphoreDelete(_mutex);
        _mutex = nullptr;
    }
}

bool InputShiftRegister::begin() {
    _mutex = xSemaphoreCreateMutex();
    if (_mutex == nullptr) {
        return false;
    }

    pinMode(_dataPin, INPUT);
    pinMode(_clockPin, OUTPUT);
    pinMode(_loadPin, OUTPUT);

    digitalWrite(_clockPin, LOW);
    digitalWrite(_loadPin, LOW);

    update();

    _initialized = true;
    return true;
}

void InputShiftRegister::read(uint8_t *data) {
    update();

    if (!takeMutex())
        return;
    for (uint8_t i = 0; i < _numChips; i++) {
        data[i] = _buffer[i];
    }
    giveMutex();
}

bool InputShiftRegister::getInput(uint8_t index) const {
    uint8_t maxBits = _numChips * 8;
    if (index >= maxBits)
        return false;

    uint8_t chip = index / 8;
    uint8_t bit = index % 8;

    return (_buffer[chip] >> bit) & 0x01;
}

void InputShiftRegister::update() {
    if (!takeMutex())
        return;
    parallelLoad();
    shiftInData();
    giveMutex();
}

/**
 * parallelLoad() - Uebernimmt Eingaenge in die CD4021-Latches
 *
 * Timing laut Datenblatt (CD4021B bei 5V, bei 3.3V konservativer):
 * - t_WH (P/S High Pulse Width): min 80 ns (typ.), 160 ns (max @ 5V)
 * - t_REM (P/S Removal -> CLK):  min 70 ns (typ. @ 10V)
 *
 * WICHTIG: CD4021BE hat invertierte Load-Logik!
 *   P/S = HIGH -> Parallel Load (Daten einlesen)
 *   P/S = LOW  -> Serial Shift  (Daten schieben)
 *
 * Wir verwenden 2 us Delays fuer Robustheit bei 3.3V (Faktor ~12x Sicherheit).
 */
void InputShiftRegister::parallelLoad() {
    digitalWrite(_loadPin, HIGH);
    delayMicroseconds(2);  // t_WH: Load-Puls (min 160 ns @ 5V)
    digitalWrite(_loadPin, LOW);
    delayMicroseconds(2);  // t_REM: Warten bis Q8 stabil (min 140 ns @ 5V)
}

/**
 * shiftInData() - Liest alle Bits von der CD4021-Kaskade
 *
 * KRITISCH: Nach Parallel Load liegt Q8 SOFORT am Ausgang!
 * Korrekte Sequenz (aus v1.1.1 Hardwaretest):
 *
 *   Load -> Lesen -> Clock -> Lesen -> Clock -> ... -> Lesen (KEIN Clock am Ende!)
 *
 * Deshalb funktioniert Hardware-SPI hier NICHT - SPI clockt vor dem Lesen.
 *
 * Der Stream wird MSB-first aufgebaut: erstes gelesenes Bit = hoechstes Bit.
 */
void InputShiftRegister::shiftInData() {
    const uint8_t totalBits = _numChips * 8;

    // Temporaerer Buffer fuer alle Bits
    uint8_t tempBuffer[MAX_SHIFT_REGS];
    memset(tempBuffer, 0, sizeof(tempBuffer));

    for (uint8_t i = 0; i < totalBits; i++) {
        // ERST lesen (Q8 enthaelt aktuelles Bit)
        uint8_t bit = digitalRead(_dataPin);

        // Bit-Position berechnen (MSB first: erstes Bit = hoechste Position)
        uint8_t bitIndex = totalBits - 1 - i;
        uint8_t chip = bitIndex / 8;
        uint8_t bitInChip = bitIndex % 8;

        if (bit) {
            tempBuffer[chip] |= (1 << bitInChip);
        }

        // DANN clocken (schiebt naechstes Bit nach Q8)
        // KEIN Clock nach dem letzten Bit! (v1.1.1 Logik)
        if (i < (totalBits - 1)) {
            pulseClock();
        }
    }

    // Buffer kopieren
    for (uint8_t i = 0; i < _numChips; i++) {
        _buffer[i] = tempBuffer[i];
    }
}

/**
 * pulseClock() - Clock-Puls fuer CD4021
 *
 * Timing laut Datenblatt:
 * - t_W (Clock Pulse Width): min 40 ns (typ. @ 10V)
 *
 * CD4021 ist CMOS und braucht laengere Pulse als 74HC595.
 * Wir verwenden 1 us (Faktor ~25x Sicherheit).
 */
void InputShiftRegister::pulseClock() {
    digitalWrite(_clockPin, HIGH);
    delayMicroseconds(1);  // t_W: Clock High (min 40 ns @ 10V)
    digitalWrite(_clockPin, LOW);
    delayMicroseconds(1);  // t_W: Clock Low
}

bool InputShiftRegister::takeMutex(TickType_t timeout) {
    if (_mutex == nullptr)
        return false;
    return xSemaphoreTake(_mutex, timeout) == pdTRUE;
}

void InputShiftRegister::giveMutex() {
    if (_mutex != nullptr) {
        xSemaphoreGive(_mutex);
    }
}

// =============================================================================
// ButtonManager
// =============================================================================

ButtonManager::ButtonManager(InputShiftRegister &shiftReg, uint8_t numButtons,
                             bool activeLow)
    : _shiftReg(shiftReg),
      _numButtons(min(numButtons, static_cast<uint8_t>(MAX_BUTTONS))),
      _activeLow(activeLow), _currentState(0), _previousState(0),
      _pressedFlags(0), _releasedFlags(0), _lastPressed(-1) {
    memset(_lastChangeTime, 0, sizeof(_lastChangeTime));
}

bool ButtonManager::begin() {
    if (!_shiftReg.begin()) {
        return false;
    }

    _shiftReg.update();

    uint32_t initial = 0;
    for (uint8_t i = 0; i < _numButtons && i < 32; i++) {
        if (readRawButton(i)) {
            initial |= (1UL << i);
        }
    }
    _currentState = initial;
    _previousState = initial;

    return true;
}

bool ButtonManager::update(ButtonEvent *event) {
    uint32_t now = millis();
    bool eventOccurred = false;

    _shiftReg.update();
    _previousState = _currentState;
    _pressedFlags = 0;
    _releasedFlags = 0;

    uint8_t maxCheck = min(_numButtons, static_cast<uint8_t>(32));

    for (uint8_t i = 0; i < maxCheck; i++) {
        bool raw = readRawButton(i);
        bool current = (_currentState >> i) & 0x01;

        if (raw != current) {
            if ((now - _lastChangeTime[i]) >= DEBOUNCE_TIME_MS) {
                if (raw) {
                    _currentState |= (1UL << i);
                    _pressedFlags |= (1UL << i);
                    _lastPressed = i;

                    if (!eventOccurred && event != nullptr) {
                        event->index = i;
                        event->type = ButtonEventType::PRESSED;
                        event->timestamp = now;
                        eventOccurred = true;
                    }
                } else {
                    _currentState &= ~(1UL << i);
                    _releasedFlags |= (1UL << i);

                    if (!eventOccurred && event != nullptr) {
                        event->index = i;
                        event->type = ButtonEventType::RELEASED;
                        event->timestamp = now;
                        eventOccurred = true;
                    }
                }
                _lastChangeTime[i] = now;
            }
        } else {
            _lastChangeTime[i] = now;
        }
    }

#ifdef DEBUG_BUTTONS
    static uint32_t lastDebugState = 0;
    if (_currentState != lastDebugState) {
        Serial.print("[BTN] State: 0x");
        Serial.println(_currentState, HEX);
        lastDebugState = _currentState;
    }
#endif

    return eventOccurred;
}

bool ButtonManager::wasPressed(uint8_t index) {
    if (index >= _numButtons || index >= 32)
        return false;

    bool pressed = (_pressedFlags >> index) & 0x01;
    if (pressed) {
        _pressedFlags &= ~(1UL << index);
    }
    return pressed;
}

bool ButtonManager::wasReleased(uint8_t index) {
    if (index >= _numButtons || index >= 32)
        return false;

    bool released = (_releasedFlags >> index) & 0x01;
    if (released) {
        _releasedFlags &= ~(1UL << index);
    }
    return released;
}

bool ButtonManager::isPressed(uint8_t index) const {
    if (index >= _numButtons || index >= 32)
        return false;
    return (_currentState >> index) & 0x01;
}

int8_t ButtonManager::getLastPressed() {
    int8_t last = _lastPressed;
    _lastPressed = -1;
    return last;
}

bool ButtonManager::readRawButton(uint8_t index) const {
    uint8_t bitPos;

#ifdef PROTOTYPE_MODE
    // Prototyp: Bit-Mapping verwenden
    if (USE_BUTTON_MAPPING && index < NUM_BUTTONS) {
        bitPos = BUTTON_BIT_MAP[index];
    } else {
        bitPos = index;
    }
#else
    // Produktion: Lineare Verdrahtung
    bitPos = index;
#endif

    bool state = _shiftReg.getInput(bitPos);

    if (_activeLow) {
        state = !state;
    }
    return state;
}
