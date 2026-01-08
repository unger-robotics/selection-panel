# Selection Panel: Hardware-Dokumentation

Version 2.5.2 | 10-Button Prototyp

## Komponentenübersicht

| Komponente | Typ | Anzahl | Funktion |
|------------|-----|--------|----------|
| XIAO ESP32-S3 | Mikrocontroller | 1 | Steuerlogik, USB-CDC |
| Raspberry Pi 5 | SBC + Netzteil + microSD | 1 | Server, Dashboard, Medien |
| CD4021B | 8-Bit PISO Schieberegister | 2 | Taster einlesen |
| 74HC595 | 8-Bit SIPO Schieberegister | 2 | LEDs ansteuern |
| Taster | 6×6 mm Tactile Switch | 10 | Benutzereingabe |
| LED | 5 mm, verschiedene Farben | 10 | Statusanzeige |
| Widerstand | 330 Ω – 3 kΩ | 10 | LED-Strombegrenzung |
| Widerstand | 10 kΩ | 10 | Pull-up für Taster |
| Kondensator | 100 nF (Keramik) | 4 | Stützkondensatoren (VCC) |

## Prototyp-Aufbau

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│   ┌─────────┐      ┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐              │
│   │  XIAO   │      │ 74HC595│ │ 74HC595│ │ CD4021B│ │ CD4021B│              │
│   │ ESP32S3 │      │   #0   │ │   #1   │ │   #0   │ │   #1   │              │
│   └─────────┘      └────────┘ └────────┘ └────────┘ └────────┘              │
│                        OUTPUT                 INPUT                         │
│                                                                             │
│   ○  ○  ●  ●  ●  ●  ○  ○  ●  ●    ← Widerstände (330Ω–3kΩ für LEDs)         │
│                                                                             │
│   ◐  ◐  ●  ●  ●  ●  ●  ●  ●  ●    ← 10 LEDs (weiß, blau, rot, gelb, grün)   │
│   1  2  3  4  5  6  7  8  9  10                                             │
│                                                                             │
│   ○  ○  ○  ○  ○  ○  ○  ○  ○  ○    ← Widerstände (10kΩ Pull-up)              │
│                                                                             │
│   ▣  ▣  ▣  ▣  ▣  ▣  ▣  ▣  ▣  ▣    ← 10 Taster (6×6 mm)                      │
│   1  2  3  4  5  6  7  8  9  10                                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Pin-Belegung XIAO ESP32-S3

```
                  ┌─────────────┐
                  │    USB-C    │
                  │             │
 LED_RCK    D0 ───┤ 1        14 ├─── 5V
 BTN_PS     D1 ───┤ 2        13 ├─── GND
 LED_OE     D2 ───┤ 3        12 ├─── 3V3
            D3 ───┤ 4        11 ├─── D10  LED_MOSI
            D4 ───┤ 5        10 ├─── D9   BTN_MISO
            D5 ───┤ 6         9 ├─── D8   SCK
            D6 ───┤ 7         8 ├─── D7
                  │             │
                  └─────────────┘
```

| Pin | Signal | Funktion | Ziel |
|-----|--------|----------|------|
| D0  | LED_RCK | Latch (STCP) | 74HC595 Pin 12 |
| D1  | BTN_PS | Parallel/Serial Control | CD4021B Pin 9 |
| D2  | LED_OE | Output Enable (PWM, active-low) | 74HC595 Pin 13 |
| D8  | SCK | SPI Clock | Beide Chip-Typen |
| D9  | BTN_MISO | Daten von CD4021B | CD4021B #0 Pin 3 (Q8) |
| D10 | LED_MOSI | Daten zu 74HC595 | 74HC595 #0 Pin 14 (SER) |

## Schaltplan-Übersicht

```
                            ┌──────────────────────────────────────────────────────┐
                            │                    OUTPUT (LEDs)                     │
    ┌─────────┐             │                                                      │
    │ Pi 5    │             │   ┌────────────┐  QH'     ┌────────────┐             │
    │         │  USB-C      │   │  74HC595   │─────────▶│  74HC595   │             │
    │         │◀───────────▶│   │    #0      │   SER    │    #1      │             │
    └─────────┘             │   │            │          │            │             │
                            │   │ QA-QH      │          │ QA-QB      │             │
        ┌─────────┐         │   │  ↓         │          │  ↓         │             │
        │  XIAO   │         │   │ LED 1-8    │          │ LED 9-10   │             │
        │ ESP32S3 │         │   └────────────┘          └────────────┘             │
        │         │         └──────────────────────────────────────────────────────┘
        │ D10 ────┼────────────▶ SER #0 (MOSI)
        │ D8  ────┼────────────▶ SCK (beide)
        │ D0  ────┼────────────▶ RCK (beide)
        │ D2  ────┼────────────▶ OE (beide)
        │         │
        │ D9  ◀───┼──────────── Q8 #0 (MISO)
        │ D8  ────┼────────────▶ CLK (beide)
        │ D1  ────┼────────────▶ P/S (beide)
        │         │         ┌──────────────────────────────────────────────────────┐
        └─────────┘         │                    INPUT (Taster)                    │
                            │                                                      │
                            │   ┌────────────┐          ┌────────────┐             │
                            │   │  CD4021B   │◀── DS ───│  CD4021B   │◀── DS ── 3V3│
                            │   │    #0      │    Q8    │    #1      │             │
                            │   │            │◀─────────│            │             │
                            │   │ PI 1-8     │          │ PI 1-2     │             │
                            │   │  ↑         │          │  ↑         │             │
                            │   │ BTN 1-8    │          │ BTN 9-10   │             │
                            │   └────────────┘          └────────────┘             │
                            │        │                                             │
                            │        ▼                                             │
                            │      Q8 ──▶ D9 (MISO)                                │
                            └──────────────────────────────────────────────────────┘
```

## 74HC595 Detailschaltplan (LED-Ausgang)

Der 74HC595 ist ein Serial-In/Parallel-Out Schieberegister mit Latch.

```
                         74HC595 (#0)                    74HC595 (#1)
                      ┌─────────────┐                 ┌─────────────┐
    LED 2  ◀─────────┤ 1  QB   VCC 16├─── 3V3 LED 10 ◀┤ 1  QB   VCC 16├─── 3V3
    LED 3  ◀─────────┤ 2  QC   QA  15├──▶ LED 1       ┤ 2  QC   QA  15├──▶ LED 9
    LED 4  ◀─────────┤ 3  QD   SER 14├◀── D10 (MOSI)  ┤ 3  QD   SER 14├◀── #0.QH'
    LED 5  ◀─────────┤ 4  QE   OE  13├◀── D2 (PWM)    ┤ 4  QE   OE  13├◀── D2 (PWM)
    LED 6  ◀─────────┤ 5  QF   RCK 12├◀── D0 (RCK)    ┤ 5  QF   RCK 12├◀── D0 (RCK)
    LED 7  ◀─────────┤ 6  QG   SCK 11├◀── D8 (SCK)    ┤ 6  QG   SCK 11├◀── D8 (SCK)
    LED 8  ◀─────────┤ 7  QH   CLR 10├─── 3V3         ┤ 7  QH   CLR 10├─── 3V3
          GND ───────┤ 8  GND  QH'  9├────▶ #1.SER    ┤ 8  GND  QH'  9├    (n.c.)
                     └───────┬───────┘                └───────┬───────┘
                             │                                │
                            ┴ C1                             ┴ C2
                           100nF                            100nF
                             │                                │
                            GND                              GND
```

**Wichtig:** CLR (Pin 10) auf 3V3 legen, nicht floaten lassen!

### LED-Beschaltung (Active-High)

```
    Qx (74HC595)
     │
    ┌┴┐
    │ │ 330Ω – 3kΩ
    └┬┘
     │
     ▼ LED
     │
    GND

Stromberechnung:
  330Ω:  I = (3.3V - 2.0V) / 330Ω  ≈ 4 mA (hell)
  1kΩ:   I = (3.3V - 2.0V) / 1000Ω ≈ 1.3 mA (gedimmt)
  3kΩ:   I = (3.3V - 2.0V) / 3000Ω ≈ 0.4 mA (schwach)
```

## CD4021B Detailschaltplan (Taster-Eingang)

Der CD4021B ist ein Parallel-In/Serial-Out Schieberegister.

### Pinout (aus Datenblatt)

```
              ┌──────────────┐
    PI-8  ────┤ 1  ●     16  ├──── VDD
      Q6  ────┤ 2        15  ├──── PI-7
      Q8  ────┤ 3        14  ├──── PI-6
    PI-4  ────┤ 4        13  ├──── PI-5
    PI-3  ────┤ 5        12  ├──── Q7
    PI-2  ────┤ 6        11  ├──── SERIAL IN (DS)
    PI-1  ────┤ 7        10  ├──── CLOCK
     VSS  ────┤ 8         9  ├──── P/S CONTROL
              └──────────────┘
```

### Beschaltung für 10 Taster

```
                         CD4021B (#0)                        CD4021B (#1)
                      ┌──────────────┐                    ┌──────────────┐
    BTN 1  ──────────┤ 1  PI-8  VDD 16├─── 3V3   BTN 9 ──┤ 1  PI-8  VDD 16├─── 3V3
                     ┤ 2  Q6   PI-7 15├◀── BTN 2         ┤ 2  Q6   PI-7 15├◀── BTN 10
   D9 (MISO) ◀───────┤ 3  Q8   PI-6 14├◀── BTN 3         ┤ 3  Q8   PI-6 14├    +3V3
    BTN 5  ──────────┤ 4  PI-4 PI-5 13├◀── BTN 4         ┤ 4  PI-4 PI-5 13├    +3V3
    BTN 6  ──────────┤ 5  PI-3   Q7 12├    (n.c.)        ┤ 5  PI-3   Q7 12├    (n.c.)
    BTN 7  ──────────┤ 6  PI-2   DS 11├◀── Q8 von #1     ┤ 6  PI-2   DS 11├─── 3V3
    BTN 8  ──────────┤ 7  PI-1  CLK 10├◀── D8 (SCK)      ┤ 7  PI-1  CLK 10├◀── D8 (SCK)
          GND ───────┤ 8  VSS  P/S  9├◀── D1 (PS)        ┤ 8  VSS  P/S  9├◀── D1 (PS)
                     └───────┬───────┘                   └───────┬───────┘
                             │         ◀────── Q8 (Pin 3) ───────┘
                            ┴ C3                                ┴ C4
                           100nF                               100nF
                             │                                   │
                            GND                                 GND
```

### Pin-Zuordnung Taster → CD4021B

Der CD4021B gibt Daten MSB-first aus: PI-1 erscheint zuerst (Bit 7), PI-8 zuletzt (Bit 0).

| Taster | Chip | Pin-Name | Pin-Nr | Bit im Byte |
|--------|------|----------|--------|-------------|
| BTN 1 | #0 | PI-8 | 1 | Bit 0 |
| BTN 2 | #0 | PI-7 | 15 | Bit 1 |
| BTN 3 | #0 | PI-6 | 14 | Bit 2 |
| BTN 4 | #0 | PI-5 | 13 | Bit 3 |
| BTN 5 | #0 | PI-4 | 4 | Bit 4 |
| BTN 6 | #0 | PI-3 | 5 | Bit 5 |
| BTN 7 | #0 | PI-2 | 6 | Bit 6 |
| BTN 8 | #0 | PI-1 | 7 | Bit 7 |
| BTN 9 | #1 | PI-8 | 1 | Bit 0 (Byte 1) |
| BTN 10 | #1 | PI-7 | 15 | Bit 1 (Byte 1) |

**Hinweis:** Die Firmware verwendet `bitops.h` mit der Formel `btn_bit(id) = 7 - ((id - 1) % 8)`, die diese Zuordnung abstrahiert.

### Daisy-Chain Datenfluss

```
3V3 ──▶ DS [CD4021B #1] ──▶ Q8 ──▶ DS [CD4021B #0] ──▶ Q8 ──▶ D9 (MISO) ──▶ ESP32
              │                          │
              │  BTN 9-10                │  BTN 1-8
              │  (Byte 1)                │  (Byte 0)
              │                          │
              └── Später gelesen ────────┴── Zuerst gelesen ──▶
```

**Wichtig:**

- **DS (Pin 11)** des **letzten** Chips (#1) auf **+3V3** legen
- Ungenutzte PI-x Pins von #1: ebenfalls auf **+3V3** (interne Pull-ups reichen nicht zuverlässig)
- Bei Active-Low Tastern werden durch DS=3V3 Einsen (= nicht gedrückt) nachgeschoben

### Taster-Beschaltung (Active-Low)

```
    3V3
     │
    ┌┴┐
    │ │ 10 kΩ (Pull-up)
    └┬┘
     ├───────── PIx (CD4021B)
     │
    ┌┴┐
    │ │ Taster
    └┬┘
     │
    GND

Zustand:
  - Losgelassen: PIx = HIGH (1) → in Firmware: nicht gedrückt
  - Gedrückt:    PIx = LOW  (0) → in Firmware: gedrückt
```

## SPI-Bus Verkabelung

```
ESP32-S3                74HC595 (#0 + #1)           CD4021B (#0 + #1)
────────                ─────────────────           ─────────────────
D10 (MOSI) ────────────▶ SER (Pin 14, #0)
                              │
                              └─── QH' (Pin 9, #0) ──▶ SER (Pin 14, #1)

D8 (SCK) ──────────────▶ SCK (Pin 11) ─────────────▶ CLK (Pin 10)

D9 (MISO) ◀──────────────────────────────────────── Q8 (Pin 3, #0)
                                                          │
                          DS (Pin 11, #0) ◀───────── Q8 (Pin 3, #1)
                                                          │
                          DS (Pin 11, #1) ◀───────── 3V3

D0 (RCK) ──────────────▶ RCK (Pin 12, beide 74HC595)

D1 (PS) ───────────────▶ P/S (Pin 9, beide CD4021B)

D2 (OE) ───────────────▶ OE (Pin 13, beide 74HC595)
```

## Timing-Diagramme

### CD4021B Lesevorgang

```
    P/S ─────┐     ┌─────────────────────────
             │     │
             └─────┘
                   ↑
                   Parallel Load (HIGH→LOW→HIGH)

    CLK ─────────────┐  ┌──┐  ┌──┐  ┌──┐  ┌──
                     │  │  │  │  │  │  │  │
                     └──┘  └──┘  └──┘  └──┘
                        ↑     ↑     ↑     ↑
                       Bit6  Bit5  Bit4  Bit3 ...

    Q8  ════════════╦══════╦══════╦══════╦════
                    ║ Bit7 ║ Bit6 ║ Bit5 ║ ...
                    ╚══════╩══════╩══════╩════
                    ↑
                    First-Bit (PI-1) liegt VOR erstem Clock an!
```

### 74HC595 Schreibvorgang

```
    SER ────╦══════╦══════╦══════╦══════────
            ║ Bit7 ║ Bit6 ║ Bit5 ║ ...
            ╚══════╩══════╩══════╩══════────

    SCK ─────────┐  ┌──┐  ┌──┐  ┌──┐  ┌─────
                 │  │  │  │  │  │  │  │
                 └──┘  └──┘  └──┘  └──┘
                    ↑     ↑     ↑     ↑
                   Daten bei steigender Flanke übernommen

    RCK ────────────────────────────────┐  ┌──
                                        │  │
                                        └──┘
                                           ↑
                                          Latch: Shift → Output
```

## Stromlimits

### Übersicht

| Komponente | Parameter | Wert | Bemerkung |
|------------|-----------|------|-----------|
| **ESP32-S3** | GPIO Output (max) | 40 mA | Drive Strength 3 |
| | GPIO Output (default) | 20 mA | Drive Strength 2 |
| | Gesamt alle GPIOs | 1200 mA | Summe |
| **74HC595** | Output pro Pin (max) | ±35 mA | Absolutes Maximum |
| | Output pro Pin (empfohlen) | ±6 mA | Dauerbetrieb |
| | VCC/GND gesamt | 70–75 mA | **Package-Limit!** |
| **CD4021B** | Input (pro Pin) | < 1 µA | CMOS-Eingang |
| | Output Q8 | ~1–3 mA | Für SPI ausreichend |

### Stromversorgung

| Komponente | Typischer Strom | Maximaler Strom |
|------------|-----------------|-----------------|
| ESP32-S3 | 80 mA | 500 mA (WiFi aktiv) |
| CD4021B (×2) | < 1 mA | 1 mA |
| 74HC595 (×2) | < 1 mA | 70 mA (alle Ausgänge) |
| LEDs (×10 @ 4 mA) | 40 mA | 40 mA |
| **Gesamt** | **~130 mA** | **~620 mA** |

Die USB-CDC Versorgung vom Pi liefert bis zu 500 mA. Bei mehr als 8 LEDs gleichzeitig an oder höheren Strömen: Helligkeit per PWM reduzieren oder externe 5V-Versorgung verwenden.

## Stückliste (BOM)

| Pos | Komponente | Wert/Typ | Anzahl | Bemerkung |
|-----|------------|----------|--------|-----------|
| 1 | XIAO ESP32-S3 | Seeed Studio | 1 | Mikrocontroller |
| 2 | Raspberry Pi 5 | 4GB/8GB | 1 | Mit Netzteil + microSD |
| 3 | CD4021B | DIP-16 | 2 | PISO Schieberegister |
| 4 | 74HC595 | DIP-16 | 2 | SIPO Schieberegister |
| 5 | Taster | 6×6 mm | 10 | Tactile Switch |
| 6 | LED 5mm | Verschiedene Farben | 10 | 2× weiß, 2× blau, 2× rot, 2× gelb, 2× grün |
| 7 | Widerstand | 330 Ω – 3 kΩ | 10 | LED-Vorwiderstand |
| 8 | Widerstand | 10 kΩ | 10 | Pull-up für Taster |
| 9 | Kondensator | 100 nF | 4 | Keramik, Stützkondensatoren |
| 10 | Lochrasterplatine | 100×160 mm | 1 | Oder Breadboard |
| 11 | USB-C Kabel | Daten-fähig | 1 | ESP32 ↔ Pi |

## Hardware-Eigenheiten

### First-Bit-Problem (CD4021B)

Nach dem Parallel-Load liegt das erste Bit (PI-1 → Q8) sofort am Ausgang, **bevor** der erste Clock kommt. Der ESP32 samplet aber erst **nach** der ersten Clock-Flanke. Die Firmware löst dies durch einen `digitalRead()` vor dem SPI-Transfer.

### DS-Pin des letzten CD4021B

Der DS-Pin (Serial Data Input, **Pin 11**) des **letzten** CD4021B in der Kette muss auf **+3V3** gelegt werden, nicht auf GND. Bei GND würden beim Einlesen Nullen nachgeschoben, die als "gedrückt" fehlinterpretiert werden.

### SPI-Bus Crosstalk

Wenn der ESP32 vom CD4021B liest, taktet er dabei Nullen durch den 74HC595. Die Ausgänge ändern sich erst beim Latch-Impuls, aber ein glitchender RCK-Pin könnte LEDs kurz ausschalten. Die Firmware kompensiert dies durch `LED_REFRESH_EVERY_CYCLE = true`.

### Active-Low Taster

Die Taster sind Active-Low beschaltet (gedrückt = 0, losgelassen = 1). Die Firmware invertiert dies in `bitops.h`, sodass die Logik-Schicht mit "pressed = true" arbeitet.

### Stützkondensatoren

Jeder IC benötigt einen 100 nF Keramikkondensator zwischen VCC und GND, möglichst nah am Chip. Ohne diese Kondensatoren können Störungen auf der Versorgung zu Fehlfunktionen führen.

## Skalierung auf 100 Buttons

Für das 100-Button-System werden jeweils 13 Schieberegister in Daisy-Chain benötigt:

| Komponente | 10-Button | 100-Button |
|------------|-----------|------------|
| CD4021B | 2 | 13 |
| 74HC595 | 2 | 13 |
| Taster | 10 | 100 |
| LEDs | 10 | 100 |
| R (LED) | 10 | 100 |
| R (Pull-up) | 10 | 100 |
| C (100nF) | 4 | 26 |

Die SPI-Transferzeit steigt von ~20 µs auf ~260 µs – weit unter dem 5 ms Zyklus.
