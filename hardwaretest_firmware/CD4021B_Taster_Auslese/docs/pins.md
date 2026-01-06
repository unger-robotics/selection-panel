# Pinout-Dokumentation: 74HC595 und CD4021B

## Umlöten: Zuordnungstabelle „Pin alt → Pin neu" (Prototyp BTN1…BTN10)

**Annahme ALT:** pro IC linear BTN → PI-1, PI-2, … PI-8
**ZIEL NEU (sauber/linear):** pro IC BTN → PI-8, PI-7, … PI-1 (damit beim Auslesen Bit0=BTN1, Bit1=BTN2, …)

### IC #0 (am ESP): Taster 1–8

| Taster | ALT: PI-Eingang (Pin) | NEU: PI-Eingang (Pin) |
|-------:|----------------------|----------------------|
|      1 | PI-1 (Pin 7)         | PI-8 (Pin 1)         |
|      2 | PI-2 (Pin 6)         | PI-7 (Pin 15)        |
|      3 | PI-3 (Pin 5)         | PI-6 (Pin 14)        |
|      4 | PI-4 (Pin 4)         | PI-5 (Pin 13)        |
|      5 | PI-5 (Pin 13)        | PI-4 (Pin 4)         |
|      6 | PI-6 (Pin 14)        | PI-3 (Pin 5)         |
|      7 | PI-7 (Pin 15)        | PI-2 (Pin 6)         |
|      8 | PI-8 (Pin 1)         | PI-1 (Pin 7)         |

### IC #1 (dahinter): Taster 9–10 (Rest optional später)

| Taster | ALT (typisch)  | NEU (linear)   |
|-------:|----------------|----------------|
|      9 | PI-1 (Pin 7)   | PI-8 (Pin 1)   |
|     10 | PI-2 (Pin 6)   | PI-7 (Pin 15)  |

---

## ESP32 XIAO → alle ICs (vollständige Verdrahtung)

### Übersicht: Welcher ESP32-Pin geht wohin?

| ESP32 Pin | Signal     | 74HC595 #0   | 74HC595 #1   | CD4021 #0    | CD4021 #1    |
|-----------|------------|--------------|--------------|--------------|--------------|
| D8        | SCK        | Pin 11 ✓     | Pin 11 ✓     | Pin 10 ✓     | Pin 10 ✓     |
| D9        | MISO       | –            | –            | Pin 3 (Q8) ✓ | –            |
| D10       | MOSI       | Pin 14 (SI) ✓| –            | –            | –            |
| D0        | LATCH_595  | Pin 12 ✓     | Pin 12 ✓     | –            | –            |
| D1        | LOAD_4021  | –            | –            | Pin 9 ✓      | Pin 9 ✓      |
| 3V3       | VCC/VDD    | Pin 16 ✓     | Pin 16 ✓     | Pin 16 ✓     | Pin 16 ✓     |
| GND       | GND/VSS    | Pin 8 ✓      | Pin 8 ✓      | Pin 8 ✓      | Pin 8 ✓      |

### Parallele Signale (Bus an alle ICs)

| Signal | ESP32 | Ziel                              |
|--------|-------|-----------------------------------|
| SCK    | D8    | 74HC595 #0 Pin 11, #1 Pin 11, CD4021 #0 Pin 10, #1 Pin 10 |
| RCK    | D0    | 74HC595 #0 Pin 12, #1 Pin 12      |
| P/S    | D1    | CD4021 #0 Pin 9, #1 Pin 9         |
| 3V3    | 3V3   | Alle ICs Pin 16                   |
| GND    | GND   | Alle ICs Pin 8                    |

### Kaskadierte Signale (IC zu IC)

| Von                  | Nach                 | Signal          |
|----------------------|----------------------|-----------------|
| 74HC595 #0 Pin 9     | 74HC595 #1 Pin 14    | QH' → SI        |
| CD4021 #1 Pin 3      | CD4021 #0 Pin 11     | Q8 → SERIAL IN  |

### Punkt-zu-Punkt Signale (nur ein IC)

| Signal | ESP32 | Ziel                 |
|--------|-------|----------------------|
| MOSI   | D10   | 74HC595 #0 Pin 14    |
| MISO   | D9    | CD4021 #0 Pin 3      |

---

## Fest verdrahtet (nicht vom ESP32 gesteuert)

| IC           | Pin | Signal    | Beschaltung | Funktion                    |
|--------------|----:|-----------|-------------|-----------------------------|
| 74HC595 #0   |  10 | SCLR      | → 3V3       | Kein Clear                  |
| 74HC595 #0   |  13 | G (OE)    | → GND       | Ausgänge aktiv              |
| 74HC595 #1   |  10 | SCLR      | → 3V3       | Kein Clear                  |
| 74HC595 #1   |  13 | G (OE)    | → GND       | Ausgänge aktiv              |
| CD4021 #1    |  11 | SERIAL IN | → 3V3       | Letzter IC, kein Eingang    |

---

## 74HC595 – 8-Bit Shift Register mit Output Latches

| Pin | Symbol | Funktion                          | IC #0                   | IC #1                   |
|----:|--------|-----------------------------------|-------------------------|-------------------------|
|   1 | QB     | Data Output B                     | → LED 2                 | → LED 10                |
|   2 | QC     | Data Output C                     | → LED 3                 | (ungenutzt)             |
|   3 | QD     | Data Output D                     | → LED 4                 | (ungenutzt)             |
|   4 | QE     | Data Output E                     | → LED 5                 | (ungenutzt)             |
|   5 | QF     | Data Output F                     | → LED 6                 | (ungenutzt)             |
|   6 | QG     | Data Output G                     | → LED 7                 | (ungenutzt)             |
|   7 | QH     | Data Output H                     | → LED 8                 | (ungenutzt)             |
|   8 | GND    | Ground                            | → GND                   | → GND                   |
|   9 | QH'    | Serial Data Output                | → IC#1 Pin 14           | → (IC#2 oder offen)     |
|  10 | SCLR   | Shift Register Clear (active LOW) | → 3V3 **fest**          | → 3V3 **fest**          |
|  11 | SCK    | Shift Register Clock              | ← ESP32 D8              | ← ESP32 D8              |
|  12 | RCK    | Storage Register Clock (LATCH)    | ← ESP32 D0              | ← ESP32 D0              |
|  13 | G      | Output Enable (active LOW)        | → GND **fest**          | → GND **fest**          |
|  14 | SI     | Serial Data Input                 | ← ESP32 D10             | ← IC#0 Pin 9            |
|  15 | QA     | Data Output A                     | → LED 1                 | → LED 9                 |
|  16 | VCC    | Positive Supply                   | → 3V3                   | → 3V3                   |

### LED-Zuordnung

**IC #0:** LED 1–8

| Ausgang | Pin | Bit | LED |
|---------|----:|----:|----:|
| QA      |  15 |   0 |   1 |
| QB      |   1 |   1 |   2 |
| QC      |   2 |   2 |   3 |
| QD      |   3 |   3 |   4 |
| QE      |   4 |   4 |   5 |
| QF      |   5 |   5 |   6 |
| QG      |   6 |   6 |   7 |
| QH      |   7 |   7 |   8 |

**IC #1:** LED 9–10 (Prototyp)

| Ausgang | Pin | Bit | LED |
|---------|----:|----:|----:|
| QA      |  15 |   0 |   9 |
| QB      |   1 |   1 |  10 |

---

## CD4021B – 8-Stage Static Shift Register

| Pin | Symbol    | Funktion                | IC #0                         | IC #1                   |
|----:|-----------|-------------------------|-------------------------------|-------------------------|
|   1 | PI-8      | Parallel Input 8        | ← Taster 1 via Pull-up        | ← Taster 9 via Pull-up  |
|   2 | Q6        | Output Stage 6          | (nicht genutzt)               | (nicht genutzt)         |
|   3 | Q8        | Serial Data Output      | → ESP32 D9 (MISO)             | → IC#0 Pin 11           |
|   4 | PI-4      | Parallel Input 4        | ← Taster 5 via Pull-up        | (ungenutzt)             |
|   5 | PI-3      | Parallel Input 3        | ← Taster 6 via Pull-up        | (ungenutzt)             |
|   6 | PI-2      | Parallel Input 2        | ← Taster 7 via Pull-up        | (ungenutzt)             |
|   7 | PI-1      | Parallel Input 1        | ← Taster 8 via Pull-up        | (ungenutzt)             |
|   8 | VSS       | Ground                  | → GND                         | → GND                   |
|   9 | P/S       | Parallel/Serial Control | ← ESP32 D1                    | ← ESP32 D1              |
|  10 | CLOCK     | Clock Input             | ← ESP32 D8                    | ← ESP32 D8              |
|  11 | SERIAL IN | Serial Data Input       | ← IC#1 Pin 3                  | → 3V3 **fest**          |
|  12 | Q7        | Output Stage 7          | (nicht genutzt)               | (nicht genutzt)         |
|  13 | PI-5      | Parallel Input 5        | ← Taster 4 via Pull-up        | (ungenutzt)             |
|  14 | PI-6      | Parallel Input 6        | ← Taster 3 via Pull-up        | (ungenutzt)             |
|  15 | PI-7      | Parallel Input 7        | ← Taster 2 via Pull-up        | ← Taster 10 via Pull-up |
|  16 | VDD       | Positive Supply         | → 3V3                         | → 3V3                   |

### Taster-Zuordnung (mit LSBFIRST)

**IC #0:** Taster 1–8

| Taster | → PI-Eingang | Pin | Bit |
|-------:|--------------|----:|----:|
|      1 | PI-8         |   1 |   0 |
|      2 | PI-7         |  15 |   1 |
|      3 | PI-6         |  14 |   2 |
|      4 | PI-5         |  13 |   3 |
|      5 | PI-4         |   4 |   4 |
|      6 | PI-3         |   5 |   5 |
|      7 | PI-2         |   6 |   6 |
|      8 | PI-1         |   7 |   7 |

**IC #1:** Taster 9–10 (Prototyp)

| Taster | → PI-Eingang | Pin | Bit |
|-------:|--------------|----:|----:|
|      9 | PI-8         |   1 |   0 |
|     10 | PI-7         |  15 |   1 |

---

## Kaskadierung (Blockdiagramm)

### 74HC595-Kette (LEDs)

```
                    ┌─────────────┐      ┌─────────────┐
ESP32 D10 (MOSI) ──→│ IC#0        │      │ IC#1        │
                    │ Pin 14 (SI) │      │ Pin 14 (SI) │
                    │             │      │             │
                    │ Pin 9 (QH') ├─────→│             │
                    │             │      │ Pin 9 (QH') ├──→ (IC#2 oder offen)
                    │ LED 1–8     │      │ LED 9–10    │
                    └─────────────┘      └─────────────┘
                          ↑                    ↑
ESP32 D8  (SCK)  ─────────┴────────────────────┘  (parallel)
ESP32 D0  (RCK)  ─────────┴────────────────────┘  (parallel)
```

### CD4021-Kette (Taster)

```
                    ┌─────────────┐      ┌─────────────┐
ESP32 D9 (MISO)  ←──│ IC#0        │      │ IC#1        │
                    │ Pin 3 (Q8)  │      │ Pin 3 (Q8)  │
                    │             │      │             │
                    │ Pin 11(SER) │←─────┤             │
                    │             │      │ Pin 11(SER) │←── 3V3 (fest)
                    │ Taster 1–8  │      │ Taster 9–10 │
                    └─────────────┘      └─────────────┘
                          ↑                    ↑
ESP32 D8  (SCK)  ─────────┴────────────────────┘  (parallel)
ESP32 D1  (P/S)  ─────────┴────────────────────┘  (parallel)
```

---

## Prototyp-Übersicht (10 Taster, 10 LEDs)

| Nr | Taster IC | Taster PI | Taster Pin | LED IC | LED Ausgang | LED Pin |
|---:|----------:|-----------|------------|-------:|-------------|--------:|
|  1 |         0 | PI-8      | 1          |      0 | QA          |      15 |
|  2 |         0 | PI-7      | 15         |      0 | QB          |       1 |
|  3 |         0 | PI-6      | 14         |      0 | QC          |       2 |
|  4 |         0 | PI-5      | 13         |      0 | QD          |       3 |
|  5 |         0 | PI-4      | 4          |      0 | QE          |       4 |
|  6 |         0 | PI-3      | 5          |      0 | QF          |       5 |
|  7 |         0 | PI-2      | 6          |      0 | QG          |       6 |
|  8 |         0 | PI-1      | 7          |      0 | QH          |       7 |
|  9 |         1 | PI-8      | 1          |      1 | QA          |      15 |
| 10 |         1 | PI-7      | 15         |      1 | QB          |       1 |

---

## Checkliste für Aufbau

### ESP32 XIAO Verbindungen

- [ ] D8 (SCK) → 74HC595 #0 Pin 11
- [ ] D8 (SCK) → 74HC595 #1 Pin 11
- [ ] D8 (SCK) → CD4021 #0 Pin 10
- [ ] D8 (SCK) → CD4021 #1 Pin 10
- [ ] D10 (MOSI) → 74HC595 #0 Pin 14
- [ ] D9 (MISO) ← CD4021 #0 Pin 3
- [ ] D0 (LATCH) → 74HC595 #0 Pin 12
- [ ] D0 (LATCH) → 74HC595 #1 Pin 12
- [ ] D1 (LOAD) → CD4021 #0 Pin 9
- [ ] D1 (LOAD) → CD4021 #1 Pin 9

### Kaskadierung

- [ ] 74HC595 #0 Pin 9 (QH') → 74HC595 #1 Pin 14 (SI)
- [ ] CD4021 #1 Pin 3 (Q8) → CD4021 #0 Pin 11 (SER IN)

### Fest verdrahtet

- [ ] 74HC595 #0 Pin 10 (SCLR) → 3V3
- [ ] 74HC595 #0 Pin 13 (G) → GND
- [ ] 74HC595 #1 Pin 10 (SCLR) → 3V3
- [ ] 74HC595 #1 Pin 13 (G) → GND
- [ ] CD4021 #1 Pin 11 (SER IN) → 3V3

### Versorgung (alle ICs)

- [ ] 74HC595 #0 Pin 16 → 3V3, Pin 8 → GND
- [ ] 74HC595 #1 Pin 16 → 3V3, Pin 8 → GND
- [ ] CD4021 #0 Pin 16 → 3V3, Pin 8 → GND
- [ ] CD4021 #1 Pin 16 → 3V3, Pin 8 → GND
