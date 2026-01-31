# Hardware-Dokumentation

Hardwarespezifikation fuer das Selection Panel mit Seeed Studio XIAO ESP32-S3.

## Board-Uebersicht

### Seeed Studio XIAO ESP32-S3

| Eigenschaft     | Wert                                     |
|-----------------|------------------------------------------|
| **MCU**         | ESP32-S3 (Xtensa LX7 Dual-Core, 240 MHz) |
| **Flash**       | 8 MB                                     |
| **PSRAM**       | 8 MB (optional)                          |
| **SRAM**        | 512 KB                                   |
| **WiFi**        | 802.11 b/g/n (2.4 GHz)                   |
| **Bluetooth**   | BLE 5.0                                  |
| **USB**         | USB-C (Native USB-CDC)                   |
| **Abmessungen** | 21 x 17.5 mm                             |

## Pinbelegung

### Projekt-Belegung (Selection Panel)

| Pin | GPIO  | Konstante      | Chip     | Funktion                  |
|-----|-------|----------------|----------|---------------------------|
| D0  | GPIO1 | `PIN_LED_RCK`  | 74HC595  | Latch (RCLK)              |
| D1  | GPIO2 | `PIN_BTN_PS`   | CD4021B  | Parallel/Shift            |
| D2  | GPIO3 | `PIN_LED_OE`   | 74HC595  | Output Enable (PWM)       |
| D8  | GPIO7 | `PIN_SCK`      | Beide    | SPI Clock (gemeinsam)     |
| D9  | GPIO8 | `PIN_BTN_MISO` | CD4021B  | Serial Out (Q8)           |
| D10 | GPIO9 | `PIN_LED_MOSI` | 74HC595  | Serial In (SER)           |

Definiert in: `include/config.h`

### Alle Pins (XIAO ESP32-S3)

| Pin | GPIO   | Funktion           | Projekt-Verwendung      |
|-----|--------|--------------------|-----------------------  |
| D0  | GPIO1  | ADC1_CH0, Touch    | LED_RCK (74HC595 Latch) |
| D1  | GPIO2  | ADC1_CH1, Touch    | BTN_PS (CD4021 P/S)     |
| D2  | GPIO3  | ADC1_CH2, Touch    | LED_OE (74HC595 OE)     |
| D3  | GPIO4  | ADC1_CH3, Touch    | (frei)                  |
| D4  | GPIO5  | ADC1_CH4, Touch    | (frei)                  |
| D5  | GPIO6  | ADC1_CH5, Touch    | (frei)                  |
| D6  | GPIO43 | TX (UART0)         | (reserviert)            |
| D7  | GPIO44 | RX (UART0)         | (reserviert)            |
| D8  | GPIO7  | SPI SCK            | SPI Clock (gemeinsam)   |
| D9  | GPIO8  | SPI MISO           | BTN_MISO (CD4021 Q8)    |
| D10 | GPIO9  | SPI MOSI           | LED_MOSI (74HC595 SER)  |
| 5V  | -      | 5V Eingang/Ausgang | -                       |
| GND | -      | Masse              | Gemeinsame Masse        |
| 3V3 | -      | 3.3V Ausgang       | VCC fuer Schieberegister|

### Reservierte GPIOs

| GPIO   | Grund         | Hinweis                        |
|--------|---------------|--------------------------------|
| GPIO0  | Boot Button   | Low beim Start = Download-Mode |
| GPIO19 | USB D-        | Nicht verwenden                |
| GPIO20 | USB D+        | Nicht verwenden                |
| GPIO43 | USB Serial TX | Native USB-CDC                 |
| GPIO44 | USB Serial RX | Native USB-CDC                 |

### Onboard-Komponenten

| Komponente       | GPIO   | Beschreibung            |
|------------------|--------|-------------------------|
| **User LED**     | GPIO21 | Gelbe LED (Active High) |
| **Reset Button** | -      | Hardware-Reset          |
| **Boot Button**  | GPIO0  | Boot-Mode / User Button |

## Anschlussbild

```
                    USB-C
                   ┌─────┐
         ┌─────────┤     ├─────────┐
         │  [RST]  └─────┘  [BOOT] │
         │   ●                 ●   │
    D0 ──┤ ○ LED_RCK             ○ ├── 5V
    D1 ──┤ ○ BTN_PS              ○ ├── GND
    D2 ──┤ ○ LED_OE              ○ ├── 3V3
    D3 ──┤ ○ (frei)              ○ ├── D10 LED_MOSI
    D4 ──┤ ○ (frei)              ○ ├── D9  BTN_MISO
    D5 ──┤ ○ (frei)  [LED]       ○ ├── D8  SCK
    D6 ──┤ ○ (TX)    GPIO21      ○ ├── D7  (RX)
         └─────────────────────────┘

    ● = Button
    ○ = Pin
    [LED] = Onboard User LED (GPIO21)
```

## Verbindung zu Schieberegistern

```
XIAO ESP32-S3              CD4021B (Buttons)        74HC595 (LEDs)
┌─────────────┐           ┌──────────────┐        ┌──────────────┐
│             │           │              │        │              │
│ D8 (SCK)   ─┼───────────┼─► CLK        │   ┌────┼─► SRCLK      │
│             │           │              │   │    │              │
│ D1 (BTN_PS)─┼───────────┼─► PS         │   │    │              │
│             │           │              │   │    │              │
│ D9 (MISO) ◄─┼───────────┼── Q8         │   │    │              │
│             │           └──────────────┘   │    │              │
│             │                              │    │              │
│ D8 (SCK)   ─┼──────────────────────────────┘    │              │
│             │                                   │              │
│ D10 (MOSI) ─┼───────────────────────────────────┼─► SER        │
│             │                                   │              │
│ D0 (RCK)   ─┼───────────────────────────────────┼─► RCLK       │
│             │                                   │              │
│ D2 (OE)    ─┼───────────────────────────────────┼─► OE         │
│             │                                   │              │
└─────────────┘                                   └──────────────┘
```

## Schieberegister

### CD4021B (Button-Input)

Parallel-In / Serial-Out Schieberegister fuer Taster-Einlesen.

| Pin   | Funktion       | Verbindung                      |
|-------|----------------|---------------------------------|
| PS    | Parallel/Shift | D1 (HIGH=Load, LOW=Shift)       |
| CLK   | Clock          | D8 (SPI SCK)                    |
| Q8    | Serial Out     | D9 (MISO)                       |
| P1-P8 | Parallel In    | Taster (Active-Low, 10k Pull-Up)|
| VCC   | Versorgung     | 3.3V                            |
| GND   | Masse          | GND                             |

**SPI-Einstellungen:**
- Frequenz: 500 kHz
- Mode: SPI_MODE1 (CPOL=0, CPHA=1)

**Kaskadierung:** Q8 von Chip N → DS von Chip N+1

### 74HC595 (LED-Output)

Serial-In / Parallel-Out Schieberegister fuer LED-Ansteuerung.

| Pin   | Funktion     | Verbindung                |
|-------|--------------|---------------------------|
| SER   | Serial In    | D10 (MOSI)                |
| SRCLK | Shift Clock  | D8 (SPI SCK)              |
| RCLK  | Latch Clock  | D0                        |
| OE    | Output Enable| D2 (Active-Low, PWM)      |
| Q0-Q7 | Parallel Out | LEDs (mit Vorwiderstand)  |
| VCC   | Versorgung   | 3.3V                      |
| GND   | Masse        | GND                       |

**SPI-Einstellungen:**
- Frequenz: 1 MHz
- Mode: SPI_MODE0 (CPOL=0, CPHA=0)

**Kaskadierung:** Q7' von Chip N → SER von Chip N+1

### Konfiguration

| Chip     | Anzahl | Kapazitaet              | Genutzt    |
|----------|--------|-------------------------|------------|
| CD4021B  | 2      | 16 Inputs               | 10 Taster  |
| 74HC595  | 2      | 16 Outputs              | 10 LEDs    |

## Beschaltung

### Taster

```
3.3V ──┬── 10kΩ ──┬── CD4021B P1
       │          │
       └── Taster ┴── GND
```

- Pull-Up: 10 kOhm extern (oder interner Pull-Up)
- Active-Low: 0 = gedrueckt, 1 = losgelassen

### LEDs

```
74HC595 Q0 ── 330Ω ──┬── LED ── GND
                     │
              (Anode)│(Kathode)
```

| LED-Farbe | Vorwiderstand | Strom   |
|-----------|---------------|---------|
| Rot       | 330 Ohm       | ~5 mA   |
| Gruen     | 220 Ohm       | ~7 mA   |
| Blau      | 220 Ohm       | ~7 mA   |
| Weiss     | 220 Ohm       | ~7 mA   |

### Abblockkondensatoren

- 100 nF keramisch je IC nahe VCC/GND
- Optional: 10 uF Elko an Versorgung

## Stromversorgung

### Eingaenge

| Quelle | Spannung  | Max. Strom |
|--------|-----------|------------|
| USB-C  | 5V        | 500 mA     |
| 5V Pin | 5V        | extern     |

### Ausgaenge

| Pin | Spannung | Max. Strom |
|-----|----------|------------|
| 3V3 | 3.3V     | 700 mA     |
| 5V  | 5V       | USB-Limit  |

### Stromverbrauch (Selection Panel)

| Komponente        | Verbrauch    |
|-------------------|--------------|
| ESP32-S3 (aktiv)  | ~50 mA       |
| 10 LEDs @ 5mA     | ~50 mA       |
| Schieberegister   | ~5 mA        |
| **Gesamt**        | **~105 mA**  |

## USB-CDC

```
USB-C Connector
├── CDC Serial (115200 Baud)
├── JTAG Debug (optional)
└── Firmware Upload
```

**Hinweis:** USB-CDC ist virtuell - Baudrate wird ignoriert, aber `config.h` definiert 115200 fuer Konsistenz.

## Boot-Mode

| GPIO0 beim Start     | Modus         |
|----------------------|---------------|
| High (normal)        | Normal Boot   |
| Low (BOOT gedrueckt) | Download Mode |

**Firmware flashen:**
1. BOOT-Taste gedrueckt halten
2. RST-Taste kurz druecken
3. BOOT-Taste loslassen
4. `pio run -t upload`

## Erweiterung auf 100 Taster

### Hardware

| Komponente | 10 Taster | 100 Taster |
|------------|-----------|------------|
| CD4021B    | 2         | 13         |
| 74HC595    | 2         | 13         |
| Taster     | 10        | 100        |
| LEDs       | 10        | 100        |
| Pull-Ups   | 10        | 100        |

### Verdrahtung (Daisy-Chain)

```
CD4021B:  [IC0] Q8 → DS [IC1] Q8 → DS [IC2] ... → DS [IC12]
                                                      ↓
                                                 D9 (MISO)

74HC595:  [IC12] ← SER [IC11] ← SER ... ← SER [IC1] ← SER [IC0]
                                                           ↑
                                                      D10 (MOSI)
```

### Stromverbrauch (100 Taster)

| Komponente         | Verbrauch    |
|--------------------|--------------|
| ESP32-S3 (aktiv)   | ~50 mA       |
| 100 LEDs @ 5mA     | ~500 mA      |
| 26 Schieberegister | ~30 mA       |
| **Gesamt**         | **~580 mA**  |

**Hinweis:** Externes 5V/1A Netzteil empfohlen bei 100 LEDs.

## Siehe auch

- `DEVELOPER.md` - Datenfluss, Timing, API
- `ALGORITHM.md` - Debounce, Selection Algorithmen
- `../CLAUDE.md` - Schnellreferenz
- `../CONTRIBUTING.md` - Coding-Standards
