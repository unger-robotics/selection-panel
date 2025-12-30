# Selection Panel - Hardware-Testphasen

Systematische Tests für das Auswahlpanel-Projekt. Jede Phase baut auf der vorherigen auf.

## Übersicht

| Phase | Ordner | Ziel | Hardware |
|-------|--------|------|----------|
| 1 | `phase1_esp32_test` | MCU verifizieren | Nur ESP32-S3 XIAO |
| 2 | `phase2_led_test` | LED-Ausgabe | + 74HC595 + LEDs |
| 3 | `phase3_button_test` | Taster-Eingabe | + CD4021BE + Taster |
| 3D | `phase3_diagnose` | Bit-Mapping ermitteln | Diagnose-Tool |
| 4 | `phase4_integration` | Taster → LED | Alles kombiniert |

## Pinbelegung ESP32-S3 XIAO

```
Signal      Pin    Ziel-IC           Funktion
──────────────────────────────────────────────────
LED Data    D0     74HC595 SER       Serielle Daten
LED Clock   D1     74HC595 SRCLK     Schiebetakt
LED Latch   D2     74HC595 RCLK      Ausgabe-Latch
Btn Data    D3     CD4021 Q8         Serielle Daten
Btn Clock   D4     CD4021 CLK        Schiebetakt
Btn Load    D5     CD4021 P/S        Parallel Load
```

---

## Phase 1: ESP32-S3 Basistest

**Ziel:** Mikrocontroller und USB-Serial prüfen

**Hardware:** Nur ESP32-S3 XIAO (USB an PC)

**Build & Flash:**

```bash
cd phase1_esp32_test
pio run -t upload -t monitor
```

**Erwartetes Verhalten:**

- Onboard-LED blinkt (500ms Takt)
- Serial zeigt "READY"
- Befehle: `PING`, `INFO`, `HELLO`, `HELP`

---

## Phase 2: LED-Test (74HC595)

**Ziel:** LED-Ausgabe über Schieberegister verifizieren

**Hardware-Aufbau:**

```
ESP32-S3 XIAO          74HC595 #0           74HC595 #1
─────────────          ──────────           ──────────
D0 ───────────────────► SER (14)
D1 ───────────────────► SRCLK (11) ◄──────► SRCLK (11)
D2 ───────────────────► RCLK (12)  ◄──────► RCLK (12)
                        QH' (9) ──────────► SER (14)
3V3 ──────────────────► VCC (16)   ◄──────► VCC (16)
3V3 ──────────────────► SRCLR (10) ◄──────► SRCLR (10)
GND ──────────────────► GND (8)    ◄──────► GND (8)
GND ──────────────────► OE (13)    ◄──────► OE (13)

LEDs: QA-QH (Pin 15, 1-7) über 330Ω nach GND
```

**Kritische Verdrahtung:**

- Pin 10 (SRCLR) → VCC (Clear deaktiviert)
- Pin 13 (OE) → GND (Ausgänge aktiviert)

**Build & Flash:**

```bash
cd phase2_led_test
pio run -t upload -t monitor
```

**Befehle:**

- `LED 5` → LED 5 ein (One-Hot)
- `ALL` → Alle LEDs ein
- `CLEAR` → Alle aus
- `CHASE` → Lauflicht
- `TEST` → Durchläuft alle LEDs
- `HELLO` → Startup-Nachricht anzeigen

---

## Phase 3: Taster-Test (CD4021BE)

**Ziel:** Taster-Eingabe über Schieberegister verifizieren

**Hardware-Aufbau:**

```
ESP32-S3 XIAO          CD4021BE #0          CD4021BE #1
─────────────          ───────────          ───────────
D3 ◄────────────────── Q8 (3)
D4 ───────────────────► CLK (10)   ◄──────► CLK (10)
D5 ───────────────────► P/S (9)    ◄──────► P/S (9)
                        SER (11) ◄──────── Q8 (3)
3V3 ──────────────────► VDD (16)   ◄──────► VDD (16)
3V3 ───────────────────────────────────────► SER (11) ← LETZTER IC!
GND ──────────────────► VSS (8)    ◄──────► VSS (8)

Taster: P1-P8 nach GND, mit 10kΩ Pull-Up nach VDD
```

**WICHTIG - CD4021 Load-Logik (invertiert zu 74HC165!):**

- P/S = HIGH → Parallel Load (Taster einlesen)
- P/S = LOW → Serial Shift (Daten auslesen)

**KRITISCH - CMOS-Eingänge:**

- SER am **LETZTEN** IC muss auf VCC!
- **Alle unbenutzten Eingänge auf VCC legen!** (CMOS-Eingänge nie floaten lassen)

**Build & Flash:**

```bash
cd phase3_button_test
pio run -t upload -t monitor
```

**Befehle:**

- `SCAN` → Einmaliges Auslesen
- `MONITOR` → Kontinuierliches Monitoring
- `RAW` → Zeigt Roh-Bits
- `HELLO` → Startup-Nachricht anzeigen

---

## Phase 3D: Diagnose (Bit-Mapping)

**Ziel:** Ermitteln welcher physische Taster welches Bit auslöst

**Wann verwenden:** Wenn Taster nicht wie erwartet reagieren oder das Mapping unklar ist.

**Build & Flash:**

```bash
cd phase3_diagnose
pio run -t upload -t monitor
```

**Verwendung:**

1. Jeden Taster einzeln drücken
2. Notieren welches Bit sich ändert
3. Mapping-Tabelle erstellen

**Befehle:**

- `HELLO` → Startup-Nachricht anzeigen
- `HELP` → Hilfe anzeigen
- Taster drücken zeigt Bit-Änderungen automatisch

**Beispiel-Ausgabe:**

```
Jetzt:  IC#1[11111111] IC#0[11111110]  -> Bit 0 LOW (GEDRUECKT)
```

**Ergebnis-Tabelle (Beispiel):**

| Taster | Bit | IC |
|--------|-----|-----|
| 1 | 15 | #1 |
| 2 | 12 | #1 |
| 3 | 13 | #1 |
| 4 | 11 | #1 |
| 5 | 10 | #1 |
| 6 | 9 | #1 |
| 7 | 8 | #1 |
| 8 | 14 | #1 |
| 9 | 7 | #0 |
| 10 | 4 | #0 |

---

## Phase 4: Integration

**Ziel:** Taster n schaltet LED n ein (1-basierte Nummerierung)

**Hardware:** Alles aus Phase 2 + Phase 3 kombiniert

**Build & Flash:**

```bash
cd phase4_integration
pio run -t upload -t monitor
```

**Verhalten:**

- Taster 1 drücken → LED 1 leuchtet
- Taster 5 drücken → LED 5 leuchtet (LED 1 aus)
- One-Hot: Nur eine LED gleichzeitig
- Kein Taster → alle LEDs aus

**Befehle:**

- `STATUS` → Zeigt Zustand
- `LED n` → Manuelle LED-Steuerung (1-10)
- `AUTO` → Auto-Modus ein/aus
- `CLEAR` → Alle LEDs aus
- `HELLO` → Startup-Nachricht anzeigen

**Bit-Mapping anpassen:**

Falls deine Verdrahtung anders ist, passe die Mapping-Tabelle in `main.cpp` an:

```cpp
constexpr uint8_t BUTTON_BIT_MAP[10] = {
    15,  // Taster 1 -> Bit 15
    12,  // Taster 2 -> Bit 12
    // ... usw.
};
```

---

## Bekannte Probleme & Lösungen

### 1. Watchdog-Reset (Task WDT triggered)

**Symptom:** ESP32 startet alle ~5 Sekunden neu mit Fehlermeldung:

```
E (13314) task_wdt: Task watchdog got triggered
E (13314) task_wdt:  - IDLE0 (CPU 0)
```

**Ursache:** ESP32-S3 mit USB-CDC braucht Zeit für den IDLE-Task.

**Lösung:** `delay(1)` am Ende der `loop()` einfügen:

```cpp
void loop() {
    // ... dein Code ...

    // WICHTIG: Watchdog füttern - ESP32-S3 USB-CDC braucht das!
    delay(1);
}
```

### 2. Taster zeigen Phantom-Drücke

**Symptom:** Taster werden als gedrückt erkannt obwohl niemand drückt.

**Ursache:** CMOS-Eingänge floaten (undefinierter Zustand).

**Lösung:**

- Alle unbenutzten Eingänge am CD4021BE direkt auf VCC (3,3V) legen
- Kein Widerstand nötig – einfach Drahtbrücke

### 3. Falsche Taster-Nummerierung

**Symptom:** Taster 1 zeigt "PRESS 9" o.ä.

**Ursache:** Verdrahtung entspricht nicht der Software-Erwartung.

**Lösung:**

1. `phase3_diagnose` flashen
2. Jeden Taster einzeln drücken und Bit notieren
3. Mapping-Tabelle in Phase 4 anpassen

### 4. LEDs bleiben an nach Phasenwechsel

**Symptom:** Nach Flash von Phase 3 leuchten LEDs noch.

**Ursache:** Phase 3 initialisiert LED-Pins nicht.

**Lösung:** Ist bereits im Code behoben – LEDs werden beim Start ausgeschaltet.

---

## Troubleshooting

### Keine Serial-Ausgabe

1. USB-C Kabel prüfen (muss Daten unterstützen)
2. Richtigen COM-Port in PlatformIO wählen
3. Nach Upload kurz warten (ESP32-S3 braucht Zeit für USB-Enumeration)

### Startup-Nachricht nicht sichtbar

**Symptom:** Nach dem Flash erscheint keine Begrüßung, aber Befehle funktionieren.

**Ursache:** ESP32-S3 bootet schneller als der Serial Monitor sich verbindet.

**Lösung:** `HELLO` eingeben – zeigt die Startup-Nachricht erneut an.

### LEDs reagieren nicht (Phase 2)

1. OE (Pin 13) muss auf GND liegen
2. SRCLR (Pin 10) muss auf VCC liegen
3. Kaskadierung: QH' → SER des nächsten ICs
4. 100nF Kondensator an jedem VCC

### Taster werden nicht erkannt (Phase 3)

1. **SER am letzten IC auf VCC!** (häufigster Fehler)
2. **Unbenutzte Eingänge auf VCC!** (zweithäufigster Fehler)
3. Pull-Up 10kΩ an jedem Taster-Eingang
4. Load-Logik: P/S = HIGH für Load (invertiert!)

### Timing-Probleme

1. delayMicroseconds in den shiftIn/shiftOut Funktionen erhöhen
2. Bei langen Leitungen: Serienwiderstand 100Ω in Clock-Leitung

---

## Schnelltest-Checkliste

### Phase 1

- [ ] USB anschließen
- [ ] `pio run -t upload -t monitor`
- [ ] "READY" erscheint (kein Watchdog-Reset)
- [ ] `PING` → `PONG`

### Phase 2

- [ ] 74HC595 verdrahtet (VCC, GND, SRCLR→VCC, OE→GND)
- [ ] LEDs mit Vorwiderständen angeschlossen
- [ ] `TEST` → LEDs laufen durch (1-10)

### Phase 3

- [ ] CD4021BE verdrahtet (VDD, VSS)
- [ ] SER→VCC am letzten IC
- [ ] **Unbenutzte Eingänge auf VCC**
- [ ] Pull-Ups an Taster-Eingängen
- [ ] `RAW` zeigt `11111111 11111111` (alle HIGH)
- [ ] `MONITOR` → Taster drücken → "PRESS n" erscheint

### Phase 4

- [ ] Beide IC-Typen kombiniert
- [ ] `AUTO` Modus aktiv
- [ ] Taster 1 → LED 1 leuchtet
- [ ] Taster 10 → LED 10 leuchtet
- [ ] Loslassen → LEDs aus

---

## Hardware-Komponenten

| Komponente | Typ | Anzahl | Funktion |
|------------|-----|--------|----------|
| MCU | ESP32-S3 XIAO | 1 | Steuerung |
| Output Shift Register | 74HC595 | 2 | LED-Ansteuerung |
| Input Shift Register | CD4021BE | 2 | Taster-Einlesen |
| LEDs | 5mm, diverse Farben | 10 | Anzeige |
| Taster | 6x6mm Tact Switch | 10 | Eingabe |
| Widerstand | 330Ω | 10 | LED-Vorwiderstand |
| Widerstand | 10kΩ | 10 | Taster-Pull-Up |
| Kondensator | 100nF | 4 | IC-Entkopplung |

---

## Bit-Mapping: Wann nötig?

### Warum Mapping beim Prototyp?

Auf dem Breadboard werden Taster oft dort angeschlossen, wo gerade Platz ist – nicht in logischer Reihenfolge. Daher braucht der Prototyp eine Mapping-Tabelle:

```cpp
constexpr uint8_t BUTTON_BIT_MAP[10] = {
    15, 12, 13, 11, 10, 9, 8, 14, 7, 4
};
```

### Für die finale PCB: Kein Mapping nötig

Bei **linearer Verdrahtung** auf der Platine entfällt das Mapping komplett:

```
Taster 1  → IC#0 P1 (Bit 0)
Taster 2  → IC#0 P2 (Bit 1)
...
Taster 8  → IC#0 P8 (Bit 7)
Taster 9  → IC#1 P1 (Bit 8)
Taster 10 → IC#1 P2 (Bit 9)
...
Taster 100 → IC#12 P4 (Bit 99)
```

Der Code vereinfacht sich dann zu:

```cpp
// Kein Mapping nötig - Taster n = Bit n-1
bool pressed = !((raw >> (taster - 1)) & 1);
```

### Übersicht

| Situation | Mapping nötig? |
|-----------|----------------|
| Prototyp auf Breadboard | Ja (kreative Verdrahtung) |
| Finale PCB mit linearer Verdrahtung | Nein |
| Anderer baut gleiche PCB | Nein (identische Hardware) |
| Anderer baut eigenen Prototyp | Ja (eigene Verdrahtung) |

### Empfehlung für PCB-Design

1. **Taster in Reihenfolge** an ICs anschließen (P1→P2→...→P8)
2. **ICs in Reihenfolge** kaskadieren (IC#0 → IC#1 → IC#2 → ...)
3. **LEDs ebenso** linear an 74HC595 anschließen (QA→QB→...→QH)

Das Diagnose-Tool (Phase 3D) bleibt nützlich für Fehlersuche oder eigene Prototypen.
