# LED-Test Firmware für Selection Panel

Hardware-SPI-Ansteuerung einer 74HC595-Schieberegisterkette am XIAO ESP32-S3.

## Übersicht

Diese Firmware demonstriert die grundlegende LED-Steuerung für das Selection-Panel-Projekt. Ein Lauflicht wandert durch 10 LEDs und validiert dabei die korrekte Verkabelung und Bit-Reihenfolge der Daisy-Chain.

```
XIAO ESP32-S3          74HC595 (IC0)         74HC595 (IC1)
┌──────────┐          ┌───────────┐         ┌───────────┐
│      SCK ├─────────►│ SRCLK     │────────►│ SRCLK     │
│     MOSI ├─────────►│ SER       │         │           │
│          │          │       Q7S ├────────►│ SER       │
│      RCK ├─────────►│ RCLK      │────────►│ RCLK      │
│       OE ├─────────►│ OE        │────────►│ OE        │
└──────────┘          └───────────┘         └───────────┘
                           │                      │
                       LED 1-8                LED 9-10
```

## Hardware-Anforderungen

| Komponente | Anzahl | Funktion |
|------------|--------|----------|
| XIAO ESP32-S3 | 1 | Mikrocontroller |
| 74HC595 | 2 | 8-Bit Schieberegister (LED-Treiber) |
| CD4021B | 2 | 8-Bit Schieberegister (Button-Eingabe, vorbereitet) |
| LEDs + Widerstände | 10 | Visuelle Ausgabe |

## Pin-Belegung

| GPIO | Signal | Funktion |
|------|--------|----------|
| D8 | SCK | SPI-Takt (gemeinsam für alle ICs) |
| D10 | MOSI | Daten zu 74HC595 |
| D0 | RCK | Latch-Signal für Ausgabeübernahme |
| D2 | OE | Output Enable mit PWM-Dimmung |
| D1 | PS | CD4021B Mode-Select (HIGH = inaktiv) |
| D9 | MISO | Daten von CD4021B (vorbereitet) |

## Funktionsweise

### Daisy-Chain-Prinzip

Wenn wir 16 Bits an zwei hintereinandergeschaltete 74HC595 senden, "rutschen" die ersten 8 Bits durch IC0 in IC1. Das bedeutet für die Sendereihenfolge:

1. **Byte[1] zuerst senden** → landet nach 16 Takten in IC1 (LED 9-16)
2. **Byte[0] zuletzt senden** → bleibt in IC0 (LED 1-8)

Der Code iteriert daher rückwärts durch das Array.

### Ghost-LED-Maskierung

Bei 10 LEDs nutzen wir nur 2 Bits im zweiten Byte. Die ungenutzten Bits 2-7 werden vor jedem Transfer auf 0 gesetzt. Ohne diese Maskierung könnten bei Hardware-Fehlern oder Rauschen "Geister-LEDs" aufleuchten.

### CD4021B-Koexistenz

Die Button-Schieberegister teilen sich den SPI-Takt mit den LEDs. Damit die LED-Kommunikation nicht versehentlich Button-Daten verschiebt, halten wir den PS-Pin (Parallel/Serial) auf HIGH. In diesem Zustand ignoriert der CD4021B den Takt für Parallel-Load.

## Konfiguration

Die Datei `config.h` enthält alle anpassbaren Parameter:

```cpp
constexpr size_t LED_COUNT = 10;        // Anzahl LEDs (skalierbar)
constexpr uint32_t SPI_HZ = 1'000'000;  // SPI-Takt (1 MHz für Breadboard)
constexpr uint8_t PWM_DUTY = 80;        // Helligkeit in Prozent
```

## Build & Upload

```bash
pio run -t upload
```

## Erwartete Ausgabe

```
=== LED-Test gestartet ===
IC0: 00000001 (0x01) | IC1: 00000000 (0x00) → LED 1 aktiv
IC0: 00000010 (0x02) | IC1: 00000000 (0x00) → LED 2 aktiv
...
IC0: 00000000 (0x00) | IC1: 00000001 (0x01) → LED 9 aktiv
IC0: 00000000 (0x00) | IC1: 00000010 (0x02) → LED 10 aktiv
```

## Nächste Schritte

- [ ] Button-Einlesen über CD4021B implementieren
- [ ] Skalierung auf 100 LEDs (13× 74HC595)
- [ ] FreeRTOS-Tasks für parallele LED/Button-Verarbeitung

## Projektstruktur

```
.
├── README.md         # Diese Dokumentation
├── platformio.ini    # Build-Konfiguration
└── src/
    ├── config.h      # Hardware-Konfiguration und Pin-Definitionen
    └── main.cpp      # Hauptprogramm mit Lauflicht-Demo
```
