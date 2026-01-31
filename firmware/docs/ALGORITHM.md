# Algorithmen

Algorithmische Spezifikationen fuer die Selection Panel Firmware.

## Debounce-Algorithmus

### Problem

Mechanische Taster prellen beim Druecken und Loslassen. Ein einzelner Tastendruck erzeugt mehrere Flanken innerhalb von 1-30 ms.

```
Tatsaechlicher Druck:     ─────┐          ┌─────
                               └──────────┘

Gemessenes Signal:        ─────┐ ┌─┐ ┌────┐ ┌─┐ ┌─────
                               └─┘ └─┘    └─┘ └─┘
                               |<--Prellen-->|
```

### Loesung: Zeitbasiertes Debouncing

Jeder Taster hat einen eigenen Timer. Der entprellte Zustand wird erst uebernommen, wenn der Rohwert fuer `DEBOUNCE_MS` (30 ms) stabil war.

### Algorithmus

```
FUER jeden Taster id = 1 bis BTN_COUNT:
    raw_now  = aktueller Rohwert
    raw_prev = Rohwert vom letzten Zyklus
    deb_now  = aktueller entprellter Wert

    WENN raw_now != raw_prev:
        # Rohwert hat sich geaendert -> Timer zuruecksetzen
        _last_change[id] = now_ms

    timer_expired = (now_ms - _last_change[id]) >= DEBOUNCE_MS
    states_differ = (raw_now != deb_now)

    WENN timer_expired UND states_differ:
        # Stabil fuer 30 ms und unterschiedlich -> uebernehmen
        deb[id] = raw_now
        any_changed = true

_raw_prev = raw  # Fuer naechsten Zyklus merken
RETURN any_changed
```

### Zeitdiagramm

```
Zeit (ms):    0    5   10   15   20   25   30   35   40
              |    |    |    |    |    |    |    |    |
Rohwert:      1    0    1    0    0    0    0    0    0
                   ▲         ▲
                   │         └── Letzter Wechsel bei t=10
                   └── Erster Wechsel bei t=5

Timer:        ─────┤    ├────┤         30ms          ├───
                   Reset     Reset                   Expired

Debounced:    1    1    1    1    1    1    1    1    0
                                                     ▲
                                                     └── Uebernahme bei t=40
```

### Warum zeitbasiert statt Counter-basiert?

| Methode | Vorteile | Nachteile |
|---------|----------|-----------|
| Counter | Einfach | Abhaengig von Abtastrate |
| Zeitbasiert | Unabhaengig von Rate | Etwas mehr Speicher |

Bei 200 Hz Abtastrate und 30 ms Debounce waeren 6 stabile Samples noetig. Mit zeitbasiertem Ansatz funktioniert derselbe Code auch bei anderer Abtastrate.

## Selection-Algorithmus

### Problem

Mehrere Taster koennen gleichzeitig gedrueckt sein. Es soll immer genau eine LED leuchten (One-Hot).

### Loesung: Last-Press-Wins mit Flanken-Erkennung

Nur steigende Flanken (nicht gedrueckt → gedrueckt) loesen einen Wechsel aus. Der zuletzt gedrueckte Taster gewinnt.

### Algorithmus

```
new_active = active_id  # Bisherige Auswahl beibehalten

FUER jeden Taster id = 1 bis BTN_COUNT:
    now_pressed  = deb_now[id] ist gedrueckt
    prev_pressed = deb_prev[id] war gedrueckt

    WENN now_pressed UND NICHT prev_pressed:
        # Steigende Flanke erkannt
        new_active = id

WENN NICHT LATCH_SELECTION UND kein Taster gedrueckt:
    new_active = 0  # Auswahl loeschen

_deb_prev = deb_now  # Fuer naechsten Zyklus merken
active_id = new_active
RETURN (active_id != prev_active)
```

### Beispiel-Szenario

```
Zeit:           t0      t1      t2      t3      t4      t5
Taster 1:       0       1       1       1       0       0
Taster 2:       0       0       1       1       1       0
Taster 3:       0       0       0       1       1       0
                        ▲       ▲       ▲
                        │       │       └── Flanke T3 -> active=3
                        │       └── Flanke T2 -> active=2
                        └── Flanke T1 -> active=1

Aktive LED:     0       1       2       3       3       3*
                                                        │
                                        *LATCH_SELECTION=true
```

### LATCH_SELECTION Verhalten

| LATCH_SELECTION | Alle losgelassen | Verhalten |
|-----------------|------------------|-----------|
| true | active_id bleibt | LED bleibt an |
| false | active_id = 0 | LED geht aus |

## Bit-Mapping

### Problem

CD4021B (Input) und 74HC595 (Output) haben unterschiedliche Bit-Reihenfolgen.

### CD4021B: MSB-first

Der CD4021B gibt Bit 7 (P1) zuerst aus, dann Bit 6 (P2), usw.

```
Physisch:     P1  P2  P3  P4  P5  P6  P7  P8
              │   │   │   │   │   │   │   │
Byte 0:      [b7][b6][b5][b4][b3][b2][b1][b0]
              │
              └── Taster 1 = Bit 7

Formel:
    byte_index = (id - 1) / 8
    bit_index  = 7 - ((id - 1) % 8)
```

### 74HC595: LSB-first

Der 74HC595 gibt Bit 0 (Q0) an den ersten Ausgang, Bit 7 (Q7) an den letzten.

```
Physisch:     Q0  Q1  Q2  Q3  Q4  Q5  Q6  Q7
              │   │   │   │   │   │   │   │
Byte 0:      [b0][b1][b2][b3][b4][b5][b6][b7]
              │
              └── LED 1 = Bit 0

Formel:
    byte_index = (id - 1) / 8
    bit_index  = (id - 1) % 8
```

### Beispiel: 10 Taster/LEDs

```
Taster-Array (2 Bytes):
    Byte 0: [T1 T2 T3 T4 T5 T6 T7 T8]  = [b7 b6 b5 b4 b3 b2 b1 b0]
    Byte 1: [T9 T10 -  -  -  -  -  - ]  = [b7 b6 b5 b4 b3 b2 b1 b0]

LED-Array (2 Bytes):
    Byte 0: [L1 L2 L3 L4 L5 L6 L7 L8]  = [b0 b1 b2 b3 b4 b5 b6 b7]
    Byte 1: [L9 L10 -  -  -  -  -  - ] = [b0 b1 b2 b3 b4 b5 b6 b7]
```

## First-Bit-Korrektur (CD4021B)

### Problem

Der CD4021B gibt nach dem Parallel-Load sofort Q8 aus, noch BEVOR der erste Clock kommt. SPI samplet aber erst NACH der ersten Clock-Flanke.

```
Parallel-Load:  PS ─────┐     ┌──────────────────
                        └─────┘

Q8 Ausgang:     ────────X─────[B1][B2][B3]...
                        ▲
                        │
                        └── Dieses Bit geht verloren!

SPI Clock:      ──────────────┐ ┐ ┐ ┐ ┐ ┐
                              └┘└┘└┘└┘└┘└┘
                              ▲
                              └── Erster Sample hier
```

### Loesung

Das erste Bit wird VOR dem SPI-Transfer mit `digitalRead()` erfasst und dann in das Ergebnis eingebaut.

### Algorithmus

```
1. PS = HIGH (Parallel Load)
2. delay 5us
3. PS = LOW (Shift Mode)
4. delay 1us
5. first_bit = digitalRead(MISO)  # KRITISCH!
6. rx[] = SPI.transfer(0x00) fuer BTN_BYTES
7. Bit-Korrektur:
   out[0] = (first_bit << 7) | (rx[0] >> 1)
   out[i] = ((rx[i-1] & 0x01) << 7) | (rx[i] >> 1)  fuer i > 0
```

### Visualisierung

```
Empfangen:     [first_bit]  [rx[0]]           [rx[1]]
                   B1      B2 B3 B4 B5 B6 B7 B8 B9  B10 ...

Ergebnis:      [out[0]]              [out[1]]
               B1 B2 B3 B4 B5 B6 B7 B8  B9 B10 ...
```

## Active-Low Logik

### Problem

Taster mit Pull-Up-Widerstaenden sind Active-Low: 0 = gedrueckt, 1 = losgelassen.

```
        3.3V
         │
        10k
         │
         ├──── CD4021B P1
         │
       Taster
         │
        GND

Offen:    P1 = HIGH (1) = losgelassen
Gedrueckt: P1 = LOW (0)  = gedrueckt
```

### Loesung

Hilfsfunktionen abstrahieren die Invertierung:

```cpp
// Prueft ob gedrueckt (invertiert)
bool activeLow_pressed(arr, id) {
    return !(arr[byte] & (1 << bit));
}

// Setzt Zustand (invertiert)
void activeLow_setPressed(arr, id, pressed) {
    if (pressed)
        arr[byte] &= ~(1 << bit);  // Bit loeschen = gedrueckt
    else
        arr[byte] |= (1 << bit);   // Bit setzen = losgelassen
}
```

## SPI Mode-Switching

### Problem

CD4021B und 74HC595 teilen den SPI-Bus, benoetigen aber unterschiedliche Modi.

| Chip | SPI Mode | CPOL | CPHA | Frequenz |
|------|----------|------|------|----------|
| CD4021B | MODE1 | 0 | 1 | 500 kHz |
| 74HC595 | MODE0 | 0 | 0 | 1 MHz |

### Loesung: SpiGuard (RAII)

```cpp
{
    SpiGuard guard(bus, SPISettings(500000, MSBFIRST, SPI_MODE1));
    // CD4021B Transfer...
}  // Automatisch: endTransaction()

{
    SpiGuard guard(bus, SPISettings(1000000, MSBFIRST, SPI_MODE0));
    // 74HC595 Transfer...
}  // Automatisch: endTransaction()
```

### Timing

```
CD4021B Read:    [──── MODE1, 500 kHz ────]

74HC595 Write:                              [── MODE0, 1 MHz ──]

Zeit:            0us                  100us                  150us
```

## Daisy-Chain Reihenfolge

### CD4021B (Input)

Daten fliessen von IC0 zu IC12. Das erste empfangene Byte enthaelt Taster 1-8.

```
MISO ◄── Q8 [IC0] ◄── DS ◄── Q8 [IC1] ◄── DS ◄── ... ◄── [IC12]
         T1-T8              T9-T16                       T97-100

SPI empfaengt: [Byte0=T1-8] [Byte1=T9-16] ... [Byte12=T97-100]
```

### 74HC595 (Output)

Daten muessen rueckwaerts gesendet werden. Das zuletzt gesendete Byte landet in IC0.

```
MOSI ──► SER [IC0] ──► Q7' ──► SER [IC1] ──► Q7' ──► ... ──► [IC12]
         L1-L8              L9-L16                        L97-100

SPI sendet: [Byte12=L97-100] ... [Byte1=L9-16] [Byte0=L1-8]
            zuerst                              zuletzt
```

### Algorithmus

```cpp
// CD4021B: Normal vorwaerts
for (i = 0; i < BTN_BYTES; i++)
    rx[i] = SPI.transfer(0x00);

// 74HC595: Rueckwaerts!
for (i = LED_BYTES - 1; i >= 0; i--)
    SPI.transfer(state[i]);
```

## Queue-Kommunikation

### Problem

IO-Task (200 Hz, Prio 5) darf nicht durch Serial-Task (Prio 2) blockiert werden.

### Loesung: Non-blocking Queue

```
IO-Task                          Serial-Task
   │                                  │
   │  log_event_t                     │
   ├─────────────────────────────────►│
   │  xQueueSend(..., 0)              │  xQueueReceive(..., 10ms)
   │  (non-blocking)                  │  (blocking mit Timeout)
   │                                  │
   │  led_cmd_event_t                 │
   │◄─────────────────────────────────┤
   │  xQueueReceive(..., 0)           │  xQueueSend(..., 0)
   │  (non-blocking)                  │  (non-blocking)
```

### Queue-Groessen

| Queue | Groesse | Element | Richtung |
|-------|---------|---------|----------|
| Log-Queue | 32 | log_event_t | IO → Serial |
| LED-Cmd-Queue | 8 | led_cmd_event_t | Serial → IO |

### Overflow-Verhalten

Bei voller Queue wird das Event verworfen (kein Blocking). Bei 200 Hz und 32 Events kann die Queue 160 ms puffern.
