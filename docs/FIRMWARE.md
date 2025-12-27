# FIRMWARE — ESP32-S3 Architektur

> Dual-Core Firmware für den IO-Controller.

| Metadaten | Wert |
|-----------|------|
| Version | 2.2.5 |
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
│   ├── config.h            # Pins, Timing, Skalierung
│   └── shift_register.h    # Hardware-Abstraktion
└── src/
    ├── main.cpp            # Setup, Loop, Tasks
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
#else
    constexpr uint8_t NUM_SHIFT_REGS = 13;  // 13 ICs pro Kette
    constexpr uint8_t NUM_LEDS = 100;
    constexpr uint8_t NUM_BUTTONS = 100;
#endif

// Timing
constexpr uint32_t DEBOUNCE_TIME_MS = 50;   // Entprellzeit
constexpr uint32_t BUTTON_POLL_MS = 10;     // Abtastintervall
constexpr uint32_t WATCHDOG_TIMEOUT_MS = 5000;
```

**Debouncing:** Mechanische Taster prellen – der Kontakt schwingt kurz, bevor er stabil ist. 50 ms Wartezeit filtert diese Störimpulse.

**Watchdog:** Ein Timer, der das System neu startet, wenn es hängt. Ohne Lebenszeichen nach 5 Sekunden → automatischer Reset.

---

## 4 Pinbelegung

| Signal | Pin | Ziel-IC | Funktion |
|--------|-----|---------|----------|
| LED Data | D0 | 74HC595 SER | Serielle Daten für LEDs |
| LED Clock | D1 | 74HC595 SRCLK | Takt für Datenübernahme |
| LED Latch | D2 | 74HC595 RCLK | Ausgabe an LEDs freigeben |
| Btn Data | D3 | CD4021 Q8 | Serielle Daten von Tastern |
| Btn Clock | D4 | CD4021 CLK | Takt zum Auslesen |
| Btn Load | D5 | CD4021 P/S | Taster-Zustände einlesen |

**Wichtig:** CD4021 verwendet `P/S = HIGH` für Parallel Load – invertiert zum bekannteren 74HC165.

---

## 5 Schieberegister-Treiber

### 5.1 LED-Ausgabe (74HC595)

```cpp
class OutputShiftRegister {
public:
    void setOnly(int8_t index);  // One-hot: nur diese LED an
    void clear();                // Alle LEDs aus
    void update();               // Buffer an Hardware senden
};
```

**One-hot:** Genau ein Bit ist aktiv – alle anderen sind aus. Bei `setOnly(5)` leuchtet nur LED 5.

### 5.2 Taster-Eingabe (CD4021)

```cpp
class InputShiftRegister {
public:
    void update();                      // Taster einlesen
    bool getInput(uint8_t index) const; // Zustand abfragen
};
```

**Mutex:** Beide Treiber schützen ihren Hardware-Zugriff mit einem Mutex – einer Sperre, die verhindert, dass zwei Tasks gleichzeitig auf dieselbe Ressource zugreifen.

---

## 6 Build und Upload

```bash
pio run                      # Kompilieren
pio run -t upload            # Flashen
pio device monitor           # Serial Monitor (115200 Baud)
pio run -t upload -t monitor # Flash + Monitor
```

---

## 7 Debugging

| Meldung | Ursache | Lösung |
|---------|---------|--------|
| `ERROR LED init failed` | Mutex-Erstellung fehlgeschlagen | RAM prüfen |
| `ERROR Queue create failed` | Nicht genug Heap | Stack-Größen reduzieren |
| Keine Button-Events | P/S-Logik falsch | HIGH für Load verwenden |

---

## 8 Features

- [x] Dual-Core: Button-Scan auf Core 0, Serial auf Core 1
- [x] Mutex-geschützter Hardware-Zugriff
- [x] Non-blocking LED-Test (State Machine)
- [x] Watchdog mit 5 s Timeout
- [x] `STATUS` liefert Heap, Queue, LED-Maske
- [x] `PROTOTYPE_MODE` für Skalierung 10 ↔ 100
