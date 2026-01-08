# Glossar

## A

### Active-High / Active-Low

Beschreibt, welcher Pegel einen Zustand "aktiviert":

- **Active-High:** Logisch 1 (HIGH, +3.3V) = aktiv. Die LED leuchtet bei HIGH.
- **Active-Low:** Logisch 0 (LOW, GND) = aktiv. Der Taster gilt als "gedrückt" bei LOW.

## B

### Bit (Binary Digit)

Die kleinste Informationseinheit: 0 oder 1. Acht Bits bilden ein Byte.

### Byte

Eine Gruppe von 8 Bits. Wertebereich: 0–255 (dezimal) oder 0x00–0xFF (hexadezimal).

## C

### CPHA (Clock Phase)

Bestimmt, bei welcher Flanke des Taktsignals Daten übernommen werden:

- CPHA=0: Daten werden bei der **ersten** Flanke übernommen
- CPHA=1: Daten werden bei der **zweiten** Flanke übernommen

Zusammen mit CPOL ergibt sich der SPI-Modus.

### CPOL (Clock Polarity)

Definiert den Ruhezustand der Taktleitung:

- CPOL=0: Takt ruht auf LOW
- CPOL=1: Takt ruht auf HIGH

## D

### Daisy-Chain (Verkettung)

Eine Methode, mehrere ICs mit wenigen Leitungen zu verbinden. Die Daten "wandern" durch die Kette wie Gänseblümchen an einer Schnur:

```
ESP32 ──► IC#0 ──► IC#1 ──► IC#2
          │        │        │
        Daten rutschen durch
```

**Vorteil:** Egal wie viele ICs – es bleiben dieselben wenigen Steuerleitungen.
**Nachteil:** Alle Daten müssen durch alle ICs geschoben werden.

### DIP (Dual In-line Package)

Ein IC-Gehäuse mit zwei parallelen Pinreihen. "DIP-16" bedeutet 16 Pins (8 pro Seite). Gut für Steckplatinen (Breadboards).

## F

### First-Bit-Problem

Eine Eigenheit des CD4021B: Nach dem Parallel-Load liegt das erste Bit sofort am Ausgang an – noch **bevor** der erste Taktimpuls kommt. SPI erwartet aber, dass sich Daten erst **nach** dem ersten Takt ändern.

**Lösung:** Das erste Bit vor dem SPI-Transfer per `digitalRead()` sichern.

## G

### GPIO (General Purpose Input/Output)

Ein frei konfigurierbarer Pin am Mikrocontroller. Kann als Eingang (Taster lesen) oder Ausgang (LED steuern) dienen.

### GND (Ground)

Das Bezugspotential der Schaltung – der "Minuspol". Alle Spannungen werden relativ zu GND gemessen.

## H

### HIGH / LOW

Die beiden logischen Pegel:

- **HIGH:** Spannung nahe VCC (z.B. 3.3V)
- **LOW:** Spannung nahe GND (0V)

### Hochohmig (Tri-State, High-Z)

Ein Ausgang, der "abgeschaltet" ist – weder HIGH noch LOW, sondern elektrisch nicht verbunden. Ermöglicht, dass mehrere ICs dieselbe Leitung nutzen können.

## I

### IC (Integrated Circuit)

Ein integrierter Schaltkreis – viele Transistoren in einem Gehäuse. Der 74HC595 enthält z.B. Schieberegister, Speicher und Ausgangstreiber.

## L

### Latch (Zwischenspeicher)

Ein Speicher, der Daten "einfriert", wenn ein Steuersignal kommt. Beim 74HC595 übernimmt der Latch-Impuls (RCK) die geschobenen Daten an die Ausgänge. Bis dahin ändern sich die Ausgänge nicht, auch wenn intern weiter geschoben wird.

### LSB (Least Significant Bit)

Das "unwichtigste" Bit – ganz rechts in der Binärdarstellung. Bei `0b00000001` ist das LSB = 1.

### LSB-first

Die Sendereihenfolge, bei der das niederwertigste Bit zuerst übertragen wird. Beim 74HC595 landet das erste gesendete Bit am Ausgang QA (Bit 0).

## M

### MISO (Master In, Slave Out)

Die SPI-Datenleitung vom Slave zum Master. Der CD4021B sendet hierüber die Taster-Zustände.

### MOSI (Master Out, Slave In)

Die SPI-Datenleitung vom Master zum Slave. Der ESP32 sendet hierüber die LED-Daten zum 74HC595.

### MSB (Most Significant Bit)

Das "wichtigste" Bit – ganz links in der Binärdarstellung. Bei `0b10000000` ist das MSB = 1.

### MSB-first

Die Sendereihenfolge, bei der das höchstwertigste Bit zuerst übertragen wird. Beim CD4021B erscheint P1 (Taster 1) als erstes Bit am Ausgang Q8.

## O

### OE (Output Enable)

Ein Steuerpin, der die Ausgänge ein- oder ausschaltet. Beim 74HC595 ist OE **active-low**: LOW = Ausgänge aktiv, HIGH = Ausgänge hochohmig (aus).

## P

### Parallel Load

Das gleichzeitige Übernehmen aller Eingänge in ein Register. Beim CD4021B geschieht dies, wenn P/S auf HIGH geht – alle 8 Tasterzustände werden "fotografiert".

### P/S (Parallel/Serial Select)

Der Umschalter zwischen zwei Betriebsmodi:

- **HIGH:** Parallel Load – Eingänge werden ins Register übernommen
- **LOW:** Serial Shift – Register wird ausgelesen

### Pull-Down / Pull-Up

Widerstände, die einen offenen Eingang auf einen definierten Pegel ziehen:

- **Pull-Down:** Zieht nach GND (LOW) – z.B. 10kΩ nach GND
- **Pull-Up:** Zieht nach VCC (HIGH) – z.B. 10kΩ nach 3.3V

Ohne diese Widerstände "floaten" offene Eingänge und lesen Rauschen.

### PWM (Pulse Width Modulation)

Eine Technik zum "Dimmen" durch schnelles Ein- und Ausschalten. Das Verhältnis von Ein- zu Auszeit (Duty Cycle) bestimmt die mittlere Helligkeit:

```
100% Duty: ████████████  (volle Helligkeit)
 50% Duty: ██████        (halbe Helligkeit)
 10% Duty: █             (sehr dunkel)
```

## Q

### Q7S (Serial Output)

Der serielle Ausgang des 74HC595. Hier "fällt" das Bit heraus, das aus dem Register geschoben wird. Wird für Daisy-Chaining mit dem SER-Eingang des nächsten ICs verbunden.

### Q8 (Serial Output, CD4021B)

Der serielle Ausgang des CD4021B. Entspricht Q7S beim 74HC595 – liefert die ausgeschobenen Bits.

## R

### RCK (Register Clock / Latch Clock)

Der Takteingang für den Latch im 74HC595. Eine steigende Flanke übernimmt den Inhalt des Schieberegisters in das Ausgangsregister.

## S

### SCK (Serial Clock) / SCLK

Die Taktleitung des SPI-Busses. Jede Flanke schiebt ein Bit weiter.

### SER (Serial Input)

Der serielle Dateneingang des 74HC595. Hier werden die Bits hineingefüttert.

### Serial Shift

Das bitweise Weiterschieben von Daten durch ein Register. Bei jedem Taktimpuls wandert jedes Bit eine Position weiter.

### Schieberegister (Shift Register)

Ein Speicher, der Daten "wie auf einem Förderband" transportiert. Bei jedem Takt rutscht alles eine Position weiter:

```
Takt 0:  [ A ][ B ][ C ][ D ]  →  Ausgang: D
Takt 1:  [ X ][ A ][ B ][ C ]  →  Ausgang: C
           ↑
       neues Bit
```

### SPI (Serial Peripheral Interface)

Ein synchrones Busprotokoll mit vier Leitungen:

| Signal | Funktion |
|--------|----------|
| SCK | Takt (vom Master) |
| MOSI | Daten Master → Slave |
| MISO | Daten Slave → Master |
| CS | Chip Select (wählt Slave) |

**Synchron** bedeutet: Der Takt wird mitgeliefert, keine Baudrate nötig.

### SPI-Modus (MODE0–MODE3)

Die Kombination aus CPOL und CPHA:

| Modus | CPOL | CPHA | Ruhepegel | Sample bei |
|-------|------|------|-----------|------------|
| MODE0 | 0 | 0 | LOW | steigender Flanke |
| MODE1 | 0 | 1 | LOW | fallender Flanke |
| MODE2 | 1 | 0 | HIGH | fallender Flanke |
| MODE3 | 1 | 1 | HIGH | steigender Flanke |

### SRCLR (Shift Register Clear)

Ein Reset-Eingang am 74HC595. Wird normalerweise auf HIGH gelegt (= nicht löschen).

### STCP (Storage Clock Pulse)

Anderer Name für RCK – der Latch-Takt.

## V

### VCC (Voltage Common Collector)

Die positive Versorgungsspannung. In dieser Schaltung: +3.3V.

### Vorwiderstand

Ein Widerstand in Reihe mit einer LED, der den Strom begrenzt. Ohne Vorwiderstand würde die LED durch zu hohen Strom zerstört.

Berechnung: R = (VCC - V_LED) / I_LED
