# Selection Panel: Button→LED mit One-Hot-Logik

Hardware-SPI-Ansteuerung von CD4021B (Taster) und 74HC595 (LEDs) am gemeinsamen Bus.

## Übersicht

Diese Firmware verbindet 10 Taster mit 10 LEDs nach dem One-Hot-Prinzip: Bei mehreren gleichzeitig gedrückten Tastern leuchtet nur die LED des zuletzt gedrückten Tasters.

```
XIAO ESP32-S3
┌─────────────┐
│         SCK ├──────────────┬──────────────────────────┐
│        MOSI ├──────────┐   │                          │
│        MISO ├──────┐   │   │                          │
│             │      │   │   │                          │
│         P/S ├──┐   │   │   │                          │
│         RCK ├──│───│───│───│───────────────────────┐  │
│          OE ├──│───│───│───│───────────────────┐   │  │
└─────────────┘  │   │   │   │                   │   │  │
                 │   │   │   │                   │   │  │
                 ▼   ▼   │   ▼                   ▼   ▼  ▼
            ┌────────────┴───────┐         ┌────────────────┐
            │ CD4021B × 2        │         │ 74HC595 × 2    │
            │ (Taster-Eingabe)   │         │ (LED-Ausgabe)  │
            └────────────────────┘         └────────────────┘
                     ▲                              │
                 Taster 1-10                    LED 1-10
```

## Hardware

| Komponente | Anzahl | Signalrichtung |
|------------|--------|----------------|
| XIAO ESP32-S3 | 1 | – |
| CD4021B | 2 | Eingang (Taster) |
| 74HC595 | 2 | Ausgang (LEDs) |
| Taster | 10 | Active-Low (Pull-Up) |
| LEDs | 10 | Active-High |

## Pin-Belegung

| GPIO | Signal | Funktion |
|------|--------|----------|
| D8 | SCK | SPI-Takt (gemeinsam) |
| D9 | MISO | Daten von CD4021B |
| D10 | MOSI | Daten zu 74HC595 |
| D1 | P/S | CD4021B Mode Select |
| D0 | RCK | 74HC595 Latch |
| D2 | OE | 74HC595 Helligkeit (PWM) |

## Zwei ICs, zwei SPI-Modi

CD4021B und 74HC595 haben unterschiedliche Timing-Anforderungen:

| IC | SPI-Modus | Begründung |
|----|-----------|------------|
| CD4021B | MODE1 | Schiebt bei steigender Flanke → Sample bei fallender |
| 74HC595 | MODE0 | Übernimmt bei steigender Flanke → Standard |

Der Code wechselt per `SPI.beginTransaction()` zwischen beiden Konfigurationen.

## Bit-Zuordnung

Die ICs haben unterschiedliche Bit-Reihenfolgen – eine häufige Fehlerquelle:

**CD4021B (Taster):** MSB-first, Active-Low

```
Byte 0: [Bit7=T1] [Bit6=T2] ... [Bit0=T8]
Byte 1: [Bit7=T9] [Bit6=T10] ...
```

**74HC595 (LEDs):** LSB-first, Active-High

```
Byte 0: [Bit0=L1] [Bit1=L2] ... [Bit7=L8]
Byte 1: [Bit0=L9] [Bit1=L10] ...
```

## One-Hot-Logik

Das Verhalten bei mehreren gedrückten Tastern:

1. **Neuer Druck:** Der zuletzt gedrückte Taster wird aktiv ("last press wins")
2. **Loslassen:** Fällt auf den kleinsten noch gedrückten Taster zurück
3. **Alle los:** Keine LED aktiv

```
Beispiel-Sequenz:
T3 drücken     → LED3 an
T7 drücken     → LED7 an (T3 noch gehalten)
T7 loslassen   → LED3 an (Fallback)
T3 loslassen   → alle aus
```

## Das "erste Bit"-Problem

Der CD4021B stellt Q8 sofort nach Parallel-Load bereit – noch vor dem ersten Clock. SPI samplet aber erst nach der ersten Flanke.

**Ohne Korrektur:** Bit 2-17 statt Bit 1-16

**Lösung im Code:**

```cpp
// 1. Erstes Bit vor SPI sichern
const uint8_t firstBit = digitalRead(PIN_BTN_MISO);

// 2. SPI-Transfer
uint8_t rx[2];
rx[0] = SPI.transfer(0x00);
rx[1] = SPI.transfer(0x00);

// 3. Korrektur: Alles um 1 nach rechts, firstBit als MSB
btnRaw[0] = (rx[0] >> 1) | (firstBit << 7);
btnRaw[1] = (rx[1] >> 1) | ((rx[0] & 0x01) << 7);
```

## Konfiguration

Die Datei `src/config.h` enthält alle Parameter:

```cpp
constexpr size_t BTN_COUNT = 10;
constexpr size_t LED_COUNT = 10;

constexpr uint32_t SPI_4021_HZ = 500000UL;   // CD4021B: 500 kHz
constexpr uint32_t SPI_595_HZ  = 1000000UL;  // 74HC595: 1 MHz

constexpr uint32_t DEBOUNCE_MS = 30;         // Entprellzeit
constexpr uint8_t PWM_DUTY     = 60;         // LED-Helligkeit in %
```

## Build & Upload

```bash
pio run -t upload
pio device monitor
```

## Erwartete Ausgabe

```
========================================
Selection Panel: Button→LED (One-Hot)
========================================
CD4021B: Bit7=T1, Bit0=T8 (Active-Low)
74HC595: Bit0=L1, Bit7=L8 (Active-High)
---
BTN RAW: IC0: 01111111 (0x7F) | IC1: 11111111 (0xFF)
BTN DEB: IC0: 01111111 (0x7F) | IC1: 11111111 (0xFF)
Active: Button=1 → LED=1
LED:     IC0: 00000001 (0x01) | IC1: 00000000 (0x00)
Pressed: 1
```

## Nächste Schritte

- [ ] Skalierung auf 100 Buttons/LEDs (13× ICs pro Kette)
- [ ] Multi-Select-Modus (mehrere LEDs gleichzeitig)
- [ ] FreeRTOS-Tasks für parallele Verarbeitung
- [ ] Event-Callbacks statt Polling

## Projektstruktur

```
.
├── README.md
├── platformio.ini
└── src/
    ├── config.h      # Hardware-Konfiguration
    └── main.cpp      # Hauptprogramm
```
