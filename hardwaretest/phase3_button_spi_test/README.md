# Taster-Test Firmware für Selection Panel

Hardware-SPI-Einlesen einer CD4021B-Schieberegisterkette am XIAO ESP32-S3.

## Übersicht

Diese Firmware demonstriert das Einlesen von Tastern über CD4021B-Schieberegister. Sie validiert die korrekte Verkabelung, Bit-Reihenfolge und löst das "erste Bit"-Problem des CD4021B bei SPI-Betrieb.

```
XIAO ESP32-S3          CD4021B (IC0)         CD4021B (IC1)
┌──────────┐          ┌───────────┐         ┌───────────┐
│      SCK ├─────────►│ CLK       │────────►│ CLK       │
│     MISO │◄─────────┤ Q8        │         │           │
│          │          │       SER ├────────►│ Q8        │
│      P/S ├─────────►│ P/S       │────────►│ P/S       │
└──────────┘          └───────────┘         └───────────┘
                           ▲                      ▲
                       Taster 1-8             Taster 9-10
```

## Hardware-Anforderungen

| Komponente | Anzahl | Funktion |
|------------|--------|----------|
| XIAO ESP32-S3 | 1 | Mikrocontroller |
| CD4021B | 2 | 8-Bit Parallel-In/Serial-Out Schieberegister |
| 74HC595 | 2 | 8-Bit Serial-In/Parallel-Out (deaktiviert) |
| Taster | 10 | Eingabe (Active-Low mit Pull-Up) |

## Pin-Belegung

| GPIO | Signal | Funktion |
|------|--------|----------|
| D8 | SCK | SPI-Takt (shared) |
| D9 | MISO | Daten von CD4021B (Q8) |
| D10 | MOSI | Daten zu 74HC595 |
| D1 | P/S | CD4021B Mode Select |
| D0 | RCK | 74HC595 Latch |
| D2 | OE | 74HC595 Output Enable |

## Das "erste Bit"-Problem

Der CD4021B hat eine Eigenheit, die beim SPI-Betrieb zu Datenversatz führt:

**Das Problem:** Nach dem Parallel-Load liegt das erste Bit (Q8) sofort am Ausgang an – noch bevor der erste Clock-Impuls kommt. SPI samplet aber erst *nach* der ersten Clock-Flanke. Ohne Korrektur lesen wir Bit 2-17 statt Bit 1-16.

**Die Lösung:**

```
1. Parallel Load auslösen (P/S = HIGH → LOW)
2. Erstes Bit per digitalRead() sichern    ← Vor SPI-Transfer!
3. 16 Bits per SPI einlesen
4. Ergebnis um 1 nach rechts schieben
5. Gesichertes Bit als MSB einsetzen
```

```cpp
const uint8_t firstBit = digitalRead(PIN_BTN_MISO);
uint16_t shifted = SPI.transfer16(0x0000);
uint16_t corrected = (firstBit << 15) | (shifted >> 1);
```

## Bit-Zuordnung

Der CD4021B liefert die Eingänge in umgekehrter Reihenfolge:

| Eingang | Position im Byte | Bedeutung |
|---------|------------------|-----------|
| P1 | Bit 7 (MSB) | Taster 1 |
| P2 | Bit 6 | Taster 2 |
| ... | ... | ... |
| P8 | Bit 0 (LSB) | Taster 8 |

**Active-Low:** Gedrückt = 0, Losgelassen = 1 (Pull-Up-Widerstände an Eingängen).

## Debouncing

Mechanische Taster prellen typisch 5-20 ms. Die Firmware implementiert zeitbasiertes Debouncing:

- Jeder Taster hat einen eigenen Timer
- Zustandswechsel wird erst nach 30 ms stabiler Phase übernommen
- Schnelle Tastenfolgen werden korrekt erfasst

## Konfiguration

Die Datei `src/config.h` enthält alle anpassbaren Parameter:

```cpp
constexpr size_t BTN_COUNT = 10;         // Anzahl Taster
constexpr uint32_t SPI_HZ = 500000UL;    // SPI-Takt (500 kHz)
constexpr uint32_t DEBOUNCE_MS = 30;     // Entprellzeit
```

## Build & Upload

```bash
pio run -t upload
pio device monitor
```

## Erwartete Ausgabe

Die LEDs werden beim Start deaktiviert (OE=HIGH, Register=0x00).

```
========================================
Taster-Test: CD4021B via Hardware-SPI
========================================
Bit-Zuordnung: Bit7=T1 ... Bit0=T8
Active-Low: 0=pressed, 1=released
---
RAW: 01111111 11111111 | DEB: 01111111 11111111 → Pressed: 1
RAW: 00111111 11111111 | DEB: 00111111 11111111 → Pressed: 1 2
RAW: 11111111 11111101 | DEB: 11111111 11111101 → Pressed: 10
```

## Nächste Schritte

- [ ] Integration mit LED-Steuerung (gemeinsamer SPI-Bus)
- [ ] Skalierung auf 100 Taster (13× CD4021B)
- [ ] Event-basierte Callbacks statt Polling

## Projektstruktur

```
.
├── README.md         # Diese Dokumentation
├── platformio.ini    # Build-Konfiguration
└── src/
    ├── config.h      # Hardware-Konfiguration
    └── main.cpp      # Hauptprogramm mit Debounce-Logik
```
