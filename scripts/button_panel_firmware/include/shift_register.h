/**
 * @file shift_register.h
 * @brief Thread-sichere Klassen fuer 74HC595 (Output) und CD4021BE (Input)
 * @version 2.2.0
 * @date 2025-12-26
 *
 * Features:
 * - Mutex-geschuetzter Hardware-Zugriff
 * - Skalierbar von Prototyp (2 ICs) bis Produktion (13 ICs)
 * - Optimierte Bit-Operationen
 */

#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "config.h"

// =============================================================================
// OUTPUT SHIFT REGISTER (74HC595)
// =============================================================================

/**
 * @class OutputShiftRegister
 * @brief Thread-sichere Steuerung einer 74HC595-Kette
 *
 * Pinout 74HC595 (DIP-16):
 *   Pin 14 (SER)   - Serial Data Input
 *   Pin 11 (SRCLK) - Shift Register Clock
 *   Pin 12 (RCLK)  - Storage Register Clock (Latch)
 *   Pin 9  (QH')   - Serial Data Output (Kaskadierung)
 *   Pin 13 (OE)    - Output Enable (Active LOW) -> GND
 *   Pin 10 (SRCLR) - Shift Register Clear (Active LOW) -> VCC
 */
class OutputShiftRegister {
public:
    OutputShiftRegister(uint8_t dataPin, uint8_t clockPin,
                        uint8_t latchPin, uint8_t numChips = NUM_595_CHIPS);
    ~OutputShiftRegister();

    bool begin();
    void write(const uint8_t* data);
    void setOutput(uint8_t index, bool state);
    bool getOutput(uint8_t index) const;
    void clear();
    void setAll();
    void update();
    void setOnly(int8_t index);

    bool isInitialized() const { return _initialized; }
    uint8_t getNumChips() const { return _numChips; }

private:
    const uint8_t _dataPin;
    const uint8_t _clockPin;
    const uint8_t _latchPin;
    const uint8_t _numChips;

    // Statischer Buffer fuer maximale Konfiguration
    uint8_t _buffer[MAX_SHIFT_REGS];
    SemaphoreHandle_t _mutex;
    bool _initialized;

    void shiftOutData();
    void pulseClock();
    void pulseLatch();

    bool takeMutex(TickType_t timeout = portMAX_DELAY);
    void giveMutex();
};

// =============================================================================
// INPUT SHIFT REGISTER (CD4021BE)
// =============================================================================

/**
 * @class InputShiftRegister
 * @brief Thread-sichere Lesung einer CD4021BE-Kette
 *
 * Pinout CD4021BE (DIP-16):
 *   Pin 3  (Q8)  - Serial Data Output
 *   Pin 10 (CLK) - Clock
 *   Pin 9  (P/S) - Parallel/Serial Control
 *   Pin 11 (SER) - Serial Data Input (Kaskadierung)
 *
 * WICHTIG: Invertierte Load-Logik!
 *   P/S = HIGH -> Parallel Load
 *   P/S = LOW  -> Serial Shift
 */
class InputShiftRegister {
public:
    InputShiftRegister(uint8_t dataPin, uint8_t clockPin,
                       uint8_t loadPin, uint8_t numChips = NUM_4021_CHIPS);
    ~InputShiftRegister();

    bool begin();
    void read(uint8_t* data);
    bool getInput(uint8_t index) const;
    void update();

    const uint8_t* getBuffer() const { return _buffer; }
    bool isInitialized() const { return _initialized; }
    uint8_t getNumChips() const { return _numChips; }

private:
    const uint8_t _dataPin;
    const uint8_t _clockPin;
    const uint8_t _loadPin;
    const uint8_t _numChips;

    // Statischer Buffer fuer maximale Konfiguration
    uint8_t _buffer[MAX_SHIFT_REGS];
    SemaphoreHandle_t _mutex;
    bool _initialized;

    void parallelLoad();
    void shiftInData();
    void pulseClock();

    bool takeMutex(TickType_t timeout = portMAX_DELAY);
    void giveMutex();
};

// =============================================================================
// BUTTON EVENT STRUKTUR
// =============================================================================

enum class ButtonEventType : uint8_t {
    NONE = 0,
    PRESSED,
    RELEASED
};

struct ButtonEvent {
    uint8_t index;
    ButtonEventType type;
    uint32_t timestamp;
};

// =============================================================================
// BUTTON MANAGER (mit Debouncing, Thread-sicher)
// =============================================================================

/**
 * @class ButtonManager
 * @brief Thread-sichere Button-Verwaltung mit Debouncing
 *
 * Unterstuetzt bis zu MAX_BUTTONS (100) Taster.
 * Nutzt uint32_t Bitmasks fuer bis zu 32 Buttons,
 * bei mehr Buttons werden Arrays verwendet.
 */
class ButtonManager {
public:
    ButtonManager(InputShiftRegister& shiftReg, uint8_t numButtons,
                  bool activeLow = true);

    bool begin();
    bool update(ButtonEvent* event);
    bool wasPressed(uint8_t index);
    bool wasReleased(uint8_t index);
    bool isPressed(uint8_t index) const;
    int8_t getLastPressed();

    // Fuer Kompatibilitaet: Gibt nur die unteren 16 Bits zurueck
    uint16_t getCurrentState() const {
        return static_cast<uint16_t>(_currentState & 0xFFFF);
    }

    // Neu: Voller 32-Bit Zustand (fuer bis zu 32 Buttons)
    uint32_t getFullState() const { return _currentState; }

private:
    InputShiftRegister& _shiftReg;
    const uint8_t _numButtons;
    const bool _activeLow;

    // 32-Bit Bitmasks fuer bis zu 32 Buttons
    // Fuer 100 Buttons: erweitern auf uint8_t Array[13]
    uint32_t _currentState;
    uint32_t _previousState;
    uint32_t _lastChangeTime[MAX_BUTTONS];
    uint32_t _pressedFlags;
    uint32_t _releasedFlags;
    int8_t _lastPressed;

    bool readRawButton(uint8_t index) const;
};

#endif // SHIFT_REGISTER_H
