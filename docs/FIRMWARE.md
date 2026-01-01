# FIRMWARE — ESP32-S3 Architektur

> Dual-Core Firmware für den IO-Controller mit sofortiger LED-Reaktion.

| Metadaten | Wert |
|-----------|------|
| Version | 2.4.1 |
| Stand | 2025-01-01 |
| Status | ✅ Prototyp funktionsfähig |
| Board | Seeed XIAO ESP32-S3 |
| Framework | Arduino + FreeRTOS |

---

## 1 Überblick

Die Firmware nutzt **FreeRTOS** – ein Echtzeit-Betriebssystem, das mehrere Tasks parallel ausführt. Der ESP32-S3 hat zwei CPU-Kerne: Core 0 scannt Taster, Core 1 verarbeitet Befehle und steuert LEDs.

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32-S3 Dual-Core                      │
├─────────────────────────────┬───────────────────────────────┤
│          Core 0             │           Core 1              │
│  ┌─────────────────────┐    │    ┌─────────────────────┐    │
│  │    buttonTask       │    │    │    Arduino loop()   │    │
│  │  • Polling 10 ms    │    │    │  • Serial RX/TX     │    │
│  │  • Debouncing       │    │    │  • LED Control      │    │
│  │  • Event Detection  │    │    │  • Command Parser   │    │
│  └──────────┬──────────┘    │    └──────────┬──────────┘    │
│             └───────────────┼───────────────┘               │
│                    buttonEventQueue                         │
│                             ↓                               │
│              ┌──────────────────────────────┐               │
│              │  handleButtonEvents()        │               │
│              │  • LED sofort setzen (< 1ms) │  ← NEU v2.4.1 │
│              │  • PRESS an Pi senden        │               │
│              └──────────────────────────────┘               │
└─────────────────────────────────────────────────────────────┘
```

**Warum zwei Kerne?** Serial-Kommunikation kann blockieren. Läuft das Button-Scanning auf einem separaten Kern, gehen keine Events verloren.

---

## 2 Neu in v2.4.1: Sofortige LED-Reaktion

Die LED reagiert jetzt direkt bei Tastendruck, ohne auf den Pi-Roundtrip zu warten.

### 2.1 Ablauf

```
[Taster gedrückt]
       ↓
   Core 0: Event in Queue
       ↓
   Core 1: handleButtonEvents()
       ↓
   ┌───────────────────────────────────┐
   │  1. LED sofort setzen (< 1ms)    │  ← Lokal, kein Warten
   │  2. PRESS xxx an Pi senden       │  ← Parallel
   └───────────────────────────────────┘
       ↓
   Pi empfängt PRESS, kann LEDSET senden
       ↓
   ESP32 ignoriert redundantes LEDSET (bereits gesetzt)
```

### 2.2 Code

```cpp
static void handleButtonEvents() {
    ButtonEvent event;
    
    while (xQueueReceive(buttonEventQueue, &event, 0) == pdTRUE) {
        if (event.type == ButtonEventType::PRESSED) {
            // LED SOFORT setzen - ohne auf Pi zu warten!
            leds.setOnly(event.index);
            currentLedIndex = event.index;
        }
        
        // Dann Event an Pi senden (parallel)
        sendButtonEvent(event);
    }
}
```

### 2.3 LEDSET-Optimierung

```cpp
// In processCommand() für LEDSET:
if (currentLedIndex == targetIndex) {
    // LED bereits gesetzt, nichts tun
    sendOK();
    return;
}
```

---

## 3 Projektstruktur

```
button_panel_firmware/
├── platformio.ini          # Build-Konfiguration
├── include/
│   ├── config.h            # Pins, Timing, Skalierung, Bit-Mapping
│   └── shift_register.h    # Hardware-Abstraktion
└── src/
    ├── main.cpp            # Setup, Loop, Tasks (v2.4.1)
    └── shift_register.cpp  # Schieberegister-Treiber (v2.3.1)
```

---

## 4 Konfiguration (config.h)

```cpp
// Skalierung: true = 10 Kanäle (Prototyp), false = 100 Kanäle
#define PROTOTYPE_MODE

#ifdef PROTOTYPE_MODE
    constexpr uint8_t NUM_SHIFT_REGS = 2;   // 2 ICs pro Kette
    constexpr uint8_t NUM_LEDS = 10;
    constexpr uint8_t NUM_BUTTONS = 10;
    
    // Bit-Mapping für Prototyp-Verdrahtung
    constexpr bool USE_BUTTON_MAPPING = true;
    constexpr uint8_t BUTTON_BIT_MAP[10] = {
        15, 12, 13, 11, 10, 9, 8, 14, 7, 4
    };
#else
    constexpr uint8_t NUM_SHIFT_REGS = 13;  // 13 ICs pro Kette
    constexpr uint8_t NUM_LEDS = 100;
    constexpr uint8_t NUM_BUTTONS = 100;
    constexpr bool USE_BUTTON_MAPPING = false;
#endif

// Timing
constexpr uint32_t DEBOUNCE_TIME_MS = 50;   // Entprellzeit
constexpr uint32_t BUTTON_POLL_MS = 10;     // Abtastintervall
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 5000;
constexpr uint32_t SERIAL_BAUD = 115200;
```

**Debouncing:** Mechanische Taster prellen – der Kontakt schwingt kurz, bevor er stabil ist. 50 ms Wartezeit filtert diese Störimpulse.

**Watchdog:** Ein Timer, der das System neu startet, wenn es hängt. Ohne Lebenszeichen nach 5 Sekunden → automatischer Reset.

**Bit-Mapping:** Im Prototyp ist die Verdrahtung nicht linear. Das Mapping übersetzt physische Taster auf logische Bit-Positionen.

---

## 5 Pinbelegung

| Signal | Pin | Ziel-IC | Funktion |
|--------|-----|---------|----------|
| LED_DATA | D0 | 74HC595 Pin 14 (SER) | Serielle Daten für LEDs |
| LED_CLK | D1 | 74HC595 Pin 11 (SRCLK) | Takt für Datenübernahme |
| LED_LATCH | D2 | 74HC595 Pin 12 (RCLK) | Ausgabe an LEDs freigeben |
| BTN_DATA | D3 | CD4021 Pin 3 (Q8) | Serielle Daten von Tastern |
| BTN_CLK | D4 | CD4021 Pin 10 (CLK) | Takt zum Auslesen |
| BTN_LOAD | D5 | CD4021 Pin 9 (P/S) | Taster-Zustände einlesen |

**Wichtig:** CD4021 verwendet `P/S = HIGH` für Parallel Load – invertiert zum bekannteren 74HC165.

---

## 6 Serial-Protokoll (1-basiert!)

### 6.1 ESP32 → Raspberry Pi

| Befehl | Bedeutung | Beispiel |
|--------|-----------|----------|
| `PRESS xxx` | Taster gedrückt | `PRESS 001` ... `PRESS 100` |
| `RELEASE xxx` | Taster losgelassen | `RELEASE 001` |
| `READY` | Controller bereit | |
| `PONG` | Antwort auf PING | |
| `OK` | Befehl ausgeführt | |
| `ERROR msg` | Fehlermeldung | `ERROR Invalid LED` |

### 6.2 Raspberry Pi → ESP32

| Befehl | Bedeutung | Beispiel |
|--------|-----------|----------|
| `LEDSET xxx` | LED ein (one-hot) | `LEDSET 001` |
| `LEDON xxx` | LED ein (additiv) | `LEDON 005` |
| `LEDOFF xxx` | LED aus | `LEDOFF 005` |
| `LEDCLR` | Alle LEDs aus | |
| `LEDALL` | Alle LEDs ein | |
| `PING` | Verbindungstest | |
| `STATUS` | Zustand abfragen | |
| `TEST` | LED-Lauflicht | |
| `STOP` | LED-Test stoppen | |
| `VERSION` | Firmware-Version | |
| `HELLO` | Startup-Nachricht | |
| `QRESET` | Queue-Statistik zurücksetzen | |
| `HELP` | Befehlsliste | |

**Nummerierung:** Alle IDs sind 1-basiert (001-100), nicht 0-basiert!

### 6.3 STATUS-Ausgabe

```
LEDS 0000100000      # Bit-Vektor (MSB links)
CURLED 5             # Aktuelle LED (1-basiert, 0 = keine)
BTNS 0000000000      # Bit-Vektor
HEAP 372756          # Freier Heap (Bytes)
CORE 1               # Aktueller CPU-Core
QFREE 16             # Freie Queue-Slots
QOVFL 0              # Queue-Overflow-Zähler (nur wenn > 0)
MODE PROTOTYPE       # Build-Modus
MAPPING ON           # Bit-Mapping aktiv
```

---

## 7 Schieberegister-Treiber

### 7.1 LED-Ausgabe (74HC595)

```cpp
class OutputShiftRegister {
public:
    void setOnly(int8_t index);  // One-hot: nur diese LED an
    void clear();                // Alle LEDs aus
    void setAll();               // Alle LEDs ein
    void update();               // Buffer an Hardware senden
};
```

**One-hot:** Genau ein Bit ist aktiv – alle anderen sind aus. Bei `setOnly(5)` leuchtet nur LED 5.

### 7.2 Taster-Eingabe (CD4021)

```cpp
class InputShiftRegister {
public:
    void update();                      // Taster einlesen
    bool getInput(uint8_t index) const; // Zustand abfragen
};
```

### 7.3 CD4021 Timing (v2.3.1)

```cpp
void InputShiftRegister::parallelLoad() {
    digitalWrite(_loadPin, HIGH);
    delayMicroseconds(2);  // t_WH: min 160 ns @ 5V
    digitalWrite(_loadPin, LOW);
    delayMicroseconds(2);  // t_REM: Warten bis Q8 stabil
}

void InputShiftRegister::shiftInData() {
    for (uint8_t i = 0; i < totalBits; i++) {
        // ERST lesen (Q8 enthält aktuelles Bit)
        uint8_t bit = digitalRead(_dataPin);
        
        // DANN clocken - KEIN Clock nach letztem Bit!
        if (i < (totalBits - 1)) {
            pulseClock();
        }
    }
}
```

**Wichtig:** Nach Parallel Load liegt Q8 sofort am Ausgang. Daher: Lesen → Clock → Lesen → Clock → ... → Lesen (kein Clock am Ende).

**Mutex:** Beide Treiber schützen ihren Hardware-Zugriff mit einem Mutex – einer Sperre, die verhindert, dass zwei Tasks gleichzeitig auf dieselbe Ressource zugreifen.

---

## 8 USB-CDC und Serial.flush()

Der ESP32-S3 XIAO nutzt **USB-CDC** (USB Communications Device Class) statt eines separaten UART-Chips. Dies führt zu Fragmentierung bei schnellen Serial-Ausgaben.

**Problem:**
```cpp
// Fragmentiert - kann als " 001" oder "SS 001" ankommen
Serial.print("PRESS");
Serial.print(' ');
Serial.println(buttonId);
```

**Lösung (v2.4.0+):**
```cpp
// Atomar - immer komplett
char buffer[16];
snprintf(buffer, sizeof(buffer), "PRESS %03d", buttonId);
Serial.println(buffer);
Serial.flush();  // KRITISCH: Sofort senden!
```

`Serial.flush()` blockiert, bis alle Daten gesendet wurden. Das verhindert Fragmentierung.

---

## 9 Build und Upload

```bash
cd button_panel_firmware

# Kompilieren
pio run

# Flashen
pio run -t upload

# Serial Monitor (115200 Baud)
pio device monitor

# Flash + Monitor
pio run -t upload -t monitor

# Clean Build
pio run -t clean
```

---

## 10 Debugging

| Meldung | Ursache | Lösung |
|---------|---------|--------|
| `ERROR LED init failed` | Mutex-Erstellung fehlgeschlagen | RAM prüfen |
| `ERROR Queue create failed` | Nicht genug Heap | Stack-Größen reduzieren |
| Keine Button-Events | P/S-Logik falsch | HIGH für Load verwenden |
| Fragmentierte Serial-Daten | USB-CDC Timing | `Serial.flush()` verwenden |
| Falsche Taster-Zuordnung | Bit-Mapping | `BUTTON_BIT_MAP` prüfen |
| LED verzögert | Alte Firmware | Auf v2.4.1 updaten |

### Debug-Befehle

```bash
# Im Serial Monitor
STATUS    # Zeigt LEDs, CURLED, Buttons, Heap, Queue
VERSION   # Firmware-Version
HELLO     # Startup-Nachricht mit Pinbelegung
QRESET    # Queue-Overflow-Zähler zurücksetzen
```

---

## 11 Features

- [x] Dual-Core: Button-Scan auf Core 0, Serial auf Core 1
- [x] **NEU:** LED reagiert sofort bei Tastendruck (< 1ms)
- [x] **NEU:** Redundantes LEDSET wird ignoriert
- [x] **NEU:** CURLED in STATUS-Ausgabe
- [x] **NEU:** QRESET-Befehl
- [x] Mutex-geschützter Hardware-Zugriff
- [x] Non-blocking LED-Test (State Machine)
- [x] Watchdog mit 5 s Timeout
- [x] `STATUS` liefert Heap, Queue, LED-Maske
- [x] `PROTOTYPE_MODE` für Skalierung 10 ↔ 100
- [x] Bit-Mapping für Prototyp-Verdrahtung
- [x] 1-basierte Nummerierung (001-100)
- [x] `Serial.flush()` für zuverlässige USB-CDC Übertragung
- [x] `snprintf` für atomare Nachrichten

---

## 12 Bekannte Einschränkungen

| Problem | Status | Workaround |
|---------|--------|------------|
| USB-CDC fragmentiert | ✅ Behoben | `Serial.flush()` |
| CD4021 braucht längere Pulse | ✅ Behoben | 2µs Load, 1µs Clock |
| Nicht-lineare Prototyp-Verdrahtung | ✅ Behoben | `BUTTON_BIT_MAP` |
| LED-Reaktion langsam | ✅ Behoben | Lokale LED-Steuerung (v2.4.1) |

---

## 13 Changelog

| Version | Datum | Änderungen |
|---------|-------|------------|
| 2.4.1 | 2025-01-01 | LED sofort bei Tastendruck, CURLED, QRESET, LEDSET-Optimierung |
| 2.4.0 | 2025-12-30 | Serial.flush(), snprintf für atomare Nachrichten |
| 2.3.1 | 2025-01-01 | CD4021 Timing (2µs Load, 1µs Clock, kein Clock nach letztem Bit) |
| 2.3.0 | 2025-12-30 | Bit-Mapping, 1-basierte Kommunikation, HELLO-Befehl |
| 2.2.5 | 2025-12-27 | CD4021 Timing korrigiert |
| 2.2.0 | 2025-12-26 | Dual-Core FreeRTOS |
