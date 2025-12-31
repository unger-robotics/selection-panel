# FIRMWARE — ESP32-S3 Architektur

> Dual-Core Firmware für den IO-Controller.

| Metadaten | Wert |
|-----------|------|
| Version | 2.4.0 |
| Stand | 2025-12-30 |
| Status | ✅ Prototyp funktionsfähig |
| Board | Seeed XIAO ESP32-S3 |
| Framework | Arduino + FreeRTOS |

---

## 1 Überblick

Die Firmware nutzt **FreeRTOS** – ein Echtzeit-Betriebssystem, das mehrere Tasks parallel ausführt. Der ESP32-S3 hat zwei CPU-Kerne: Core 0 scannt Taster, Core 1 verarbeitet Befehle.

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
└─────────────────────────────────────────────────────────────┘
```

**Warum zwei Kerne?** Serial-Kommunikation kann blockieren. Läuft das Button-Scanning auf einem separaten Kern, gehen keine Events verloren.

---

## 2 Projektstruktur

```
button_panel_firmware/
├── platformio.ini          # Build-Konfiguration
├── include/
│   ├── config.h            # Pins, Timing, Skalierung, Bit-Mapping
│   └── shift_register.h    # Hardware-Abstraktion
└── src/
    ├── main.cpp            # Setup, Loop, Tasks (v2.4.0)
    └── shift_register.cpp  # Schieberegister-Treiber
```

---

## 3 Konfiguration (config.h)

```cpp
// Skalierung: true = 10 Kanäle (Prototyp), false = 100 Kanäle
#define PROTOTYPE_MODE

#ifdef PROTOTYPE_MODE
    constexpr uint8_t NUM_SHIFT_REGS = 2;   // 2 ICs pro Kette
    constexpr uint8_t NUM_LEDS = 10;
    constexpr uint8_t NUM_BUTTONS = 10;
    
    // Bit-Mapping für Prototyp-Verdrahtung
    constexpr bool USE_BUTTON_MAPPING = true;
    static const uint8_t BUTTON_BIT_MAP[10] = {
        7, 6, 5, 4, 3, 2, 1, 0,  // Taster 1-8 → IC 0
        15, 14                    // Taster 9-10 → IC 1
    };
#else
    constexpr uint8_t NUM_SHIFT_REGS = 13;  // 13 ICs pro Kette
    constexpr uint8_t NUM_LEDS = 100;
    constexpr uint8_t NUM_BUTTONS = 100;
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

## 4 Pinbelegung

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

## 5 Serial-Protokoll (1-basiert!)

### 5.1 ESP32 → Raspberry Pi

| Befehl | Bedeutung | Beispiel |
|--------|-----------|----------|
| `PRESS xxx` | Taster gedrückt | `PRESS 001` ... `PRESS 100` |
| `RELEASE xxx` | Taster losgelassen | `RELEASE 001` |
| `READY` | Controller bereit | |
| `PONG` | Antwort auf PING | |
| `OK` | Befehl ausgeführt | |
| `ERROR msg` | Fehlermeldung | `ERROR Invalid LED` |

### 5.2 Raspberry Pi → ESP32

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
| `HELLO` | Startup-Nachricht | |

**Nummerierung:** Alle IDs sind 1-basiert (001-100), nicht 0-basiert!

---

## 6 Schieberegister-Treiber

### 6.1 LED-Ausgabe (74HC595)

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

### 6.2 Taster-Eingabe (CD4021)

```cpp
class InputShiftRegister {
public:
    void update();                      // Taster einlesen
    bool getInput(uint8_t index) const; // Zustand abfragen
};
```

**Mutex:** Beide Treiber schützen ihren Hardware-Zugriff mit einem Mutex – einer Sperre, die verhindert, dass zwei Tasks gleichzeitig auf dieselbe Ressource zugreifen.

---

## 7 USB-CDC und Serial.flush()

Der ESP32-S3 XIAO nutzt **USB-CDC** (USB Communications Device Class) statt eines separaten UART-Chips. Dies führt zu Fragmentierung bei schnellen Serial-Ausgaben.

**Problem:**
```cpp
// Fragmentiert - kann als " 001" oder "SS 001" ankommen
Serial.print("PRESS");
Serial.print(' ');
Serial.println(buttonId);
```

**Lösung (v2.4.0):**
```cpp
// Atomar - immer komplett
char buffer[16];
snprintf(buffer, sizeof(buffer), "PRESS %03d", buttonId);
Serial.println(buffer);
Serial.flush();  // KRITISCH: Sofort senden!
```

`Serial.flush()` blockiert, bis alle Daten gesendet wurden. Das verhindert Fragmentierung.

---

## 8 Build und Upload

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

## 9 Debugging

| Meldung | Ursache | Lösung |
|---------|---------|--------|
| `ERROR LED init failed` | Mutex-Erstellung fehlgeschlagen | RAM prüfen |
| `ERROR Queue create failed` | Nicht genug Heap | Stack-Größen reduzieren |
| Keine Button-Events | P/S-Logik falsch | HIGH für Load verwenden |
| Fragmentierte Serial-Daten | USB-CDC Timing | `Serial.flush()` verwenden |
| Falsche Taster-Zuordnung | Bit-Mapping | `BUTTON_BIT_MAP` prüfen |

### Debug-Befehle

```bash
# Im Serial Monitor
STATUS    # Zeigt LEDs, Buttons, Heap, Queue
VERSION   # Firmware-Version
HELLO     # Startup-Nachricht mit Pinbelegung
```

---

## 10 Features

- [x] Dual-Core: Button-Scan auf Core 0, Serial auf Core 1
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

## 11 Bekannte Einschränkungen

| Problem | Status | Workaround |
|---------|--------|------------|
| USB-CDC fragmentiert | ✅ Behoben | `Serial.flush()` |
| CD4021 braucht längere Pulse | ✅ Behoben | `delayMicroseconds(5)` |
| Nicht-lineare Prototyp-Verdrahtung | ✅ Behoben | `BUTTON_BIT_MAP` |

---

## 12 Changelog

| Version | Datum | Änderungen |
|---------|-------|------------|
| 2.4.0 | 2025-12-30 | Serial.flush(), snprintf für atomare Nachrichten |
| 2.3.0 | 2025-12-30 | Bit-Mapping, 1-basierte Kommunikation, HELLO-Befehl |
| 2.2.5 | 2025-12-27 | CD4021 Timing korrigiert |
| 2.2.0 | 2025-12-26 | Dual-Core FreeRTOS |
