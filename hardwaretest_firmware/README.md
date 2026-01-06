# Selection Panel - Hardware Test Firmware

ESP32-S3 Firmware für 10-100 Taster/LED Selection Panel.

## Projektstruktur

```
├── phase1_esp32_test/      ESP32 Threading (Arduino vs FreeRTOS)
├── phase2_led_test/        LED-Ausgabe über 74HC595
├── phase3_button_test/     Taster-Eingabe über CD4021B
├── phase4_combined_test/   Kombiniert (Taster → LED)
├── phase5_debouncing/      Entprellen (Simple vs Advanced)
├── phase6_esp32_pi5/       [TODO] ESP32 ↔ Pi5 Kommunikation
└── README.md
```

## Hardware

| Komponente | IC | Anzahl (10) | Anzahl (100) |
|------------|-----|-------------|--------------|
| Taster | CD4021B | 2 | 13 |
| LEDs | 74HC595 | 2 | 13 |
| MCU | ESP32-S3 | 1 | 1 |

## Pinbelegung (XIAO ESP32-S3)

| Pin | Funktion | IC |
|-----|----------|-----|
| D8 | SCK (Clock) | Alle |
| D1 | P/S (Load) | CD4021B |
| D9 | MISO (Data In) | CD4021B |
| D10 | MOSI (Data Out) | 74HC595 |
| D0 | RCK (Latch) | 74HC595 |
| D2 | OE (PWM) | 74HC595 |

## Phasen

### Phase 1: ESP32 Threading
Vergleich Arduino (Single-Thread) vs FreeRTOS (Multi-Thread).

**Erkenntnis:** USB-CDC läuft auf Core 0 → User-Tasks auf Core 1, Message Queue für Serial.

### Phase 2: LED-Test (74HC595)
Lauflicht zur Hardware-Verifikation.

```
LED: LSB-basiert (Bit 0 = LED 1)
L1=0x01  L2=0x02  L3=0x04  L4=0x08
L5=0x10  L6=0x20  L7=0x40  L8=0x80
```

### Phase 3: Taster-Test (CD4021B)
Taster-Eingabe mit Active-Low Logik.

```
BTN: MSB-basiert (Bit 7 = Taster 1)
T1=0x7F  T2=0xBF  T3=0xDF  T4=0xEF
T5=0xF7  T6=0xFB  T7=0xFD  T8=0xFE
```

### Phase 4: Kombiniert
Taster drücken → entsprechende LED leuchtet.

```
T 1->L 1 | IC#0 | BTN:Bit7(MSB) LED:Bit0(LSB) | 0x7F->0x01
```

### Phase 5: Debouncing
Zwei Varianten zum Entprellen mechanischer Taster.

| Variante | Beschreibung | Für |
|----------|--------------|-----|
| Simple | Zeitbasiert (30ms stabil) | Prototyp |
| Advanced | State Machine mit Events | Produktion |

```
T 1 PRESS   -> LED ON
T 1 RELEASE
```

### Phase 6: ESP32 ↔ Pi5 (TODO)
Kommunikation zwischen ESP32 und Raspberry Pi 5.

- micro-ROS / ROS 2 Humble
- USB-Serial oder WiFi
- Bidirektionale Steuerung

## Skalierung auf 100

In `config.h`:

```cpp
constexpr size_t BTN_COUNT = 100;  // statt 10
constexpr size_t LED_COUNT = 100;  // statt 10
```

Der Rest wird automatisch berechnet.

## Build & Flash

```bash
cd phase5_debouncing/phase5_advanced
pio run -t upload
pio device monitor
```

## Bit-Zuordnung

| IC | Zuordnung | Shift | Grund |
|----|-----------|-------|-------|
| CD4021B | Bit 7 = ID 1 (MSB) | Q8 zuerst | Hardware-Verdrahtung |
| 74HC595 | Bit 0 = ID 1 (LSB) | QA = Pin 15 | Hardware-Verdrahtung |

Die API abstrahiert das:

```cpp
isPressed(5);      // Prüft Taster 5 (intern Bit 3)
setLED(5, true);   // Schaltet LED 5 (intern Bit 4)
```

## Status

- [x] Phase 1: ESP32 Threading
- [x] Phase 2: LED-Ausgabe (74HC595)
- [x] Phase 3: Taster-Eingabe (CD4021B)
- [x] Phase 4: Kombiniert (Taster → LED)
- [x] Phase 5: Debouncing (Simple + Advanced)
- [ ] Phase 6: ESP32 ↔ Pi5 Kommunikation
- [ ] Skalierung auf 100 Taster/LEDs
