# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Projektübersicht

Selection Panel ist ein modulares Eingabesystem mit 10-100 physischen Tastern und LED-Feedback. Ein ESP32-S3 (XIAO) übernimmt die Echtzeit-I/O (200 Hz) über Schieberegister, während ein Raspberry Pi 5 die Anwendungslogik ausführt (Medienwiedergabe, Web-Dashboard).

```
Raspberry Pi 5          USB-CDC @ 115200        ESP32-S3 XIAO
(aiohttp + WebSocket) ◀───────────────────────▶ (FreeRTOS)
       │                    PRESS 001              │
       │                    LEDSET 001             │ SPI-Bus
       ▼                                           ▼
Browser Dashboard                           CD4021B (Taster)
                                            74HC595 (LEDs)
```

## Projektstruktur

```
selection-panel/
├── firmware/           # ESP32-S3 Firmware (PlatformIO) → siehe firmware/CLAUDE.md
├── server/             # Python Backend
│   ├── server.py
│   └── media/          # Medien-Dateien (001.jpg, 001.mp3, ...)
├── dashboard/          # Web Frontend
│   ├── index.html
│   └── static/         # app.js, styles.css
├── doc/                # Dokumentation
├── scripts/            # Hilfsskripte
└── hardwaretest/       # Hardware-Testphasen
```

Für detaillierte Firmware-Entwicklung siehe `firmware/CLAUDE.md` (Timing-Budget, Coding-Standards, Doxygen).

## Build-Befehle

### ESP32 Firmware (PlatformIO)

```bash
cd firmware
pio run                       # Kompilieren
pio run -t upload             # Auf Gerät flashen
pio device monitor            # Serial-Monitor (115200 Baud)
pio run -t upload -t monitor  # Build + Flash + Monitor
pio run -e debug              # Debug-Build (CORE_DEBUG_LEVEL=4)
pio run -t clean              # Clean Build

# Code-Qualität
./tools/format.sh             # Code formatieren (clang-format)
./tools/lint.sh               # Statische Analyse (cppcheck)
```

### Python-Server (Raspberry Pi)

```bash
cd ~/selection-panel/server
source ../venv/bin/activate
python server.py           # Server starten auf Port 8080
# Dashboard: http://rover:8080/ (rover = Pi Hostname)

# Erstmaliges Setup (auf Pi)
python3 -m venv ~/selection-panel/venv
source ../venv/bin/activate
pip install aiohttp pyserial
```

### Hilfsskripte

```bash
./scripts/generate_test_media.sh      # Erzeugt server/media/001-010.jpg und .mp3
./scripts/optimize-project-images.sh  # Bilder für Web optimieren
./scripts/rename_images.sh            # Bilder umbenennen (001.jpg, ...)
./scripts/rename_sounds.sh            # Audio umbenennen (001.mp3, ...)
./scripts/convert_mov_to_mp4.sh       # MOV → MP4 konvertieren
```

### HTTP-Endpoints testen

```bash
curl http://rover:8080/status | jq   # Server-Status
curl http://rover:8080/health        # Health-Check (200/503)
curl http://rover:8080/test/play/5   # Tastendruck simulieren (1-basiert!)
curl http://rover:8080/test/stop     # Wiedergabe stoppen
```

### Serial-Tests (auf Pi ausführen)

```bash
# ESP32-Gerät finden (stabiler by-id Pfad)
SERIAL_PORT=$(ls /dev/serial/by-id/usb-Espressif* 2>/dev/null | head -1)

# Testbefehle senden
stty -F $SERIAL_PORT 115200 raw -echo
echo "PING" > $SERIAL_PORT       # Erwartet: PONG
echo "STATUS" > $SERIAL_PORT     # Aktuellen Zustand anzeigen
echo "LEDSET 001" > $SERIAL_PORT # LED 1 setzen (One-Hot)
echo "LEDCLR" > $SERIAL_PORT     # Alle LEDs aus

# Interaktiver Modus
screen $SERIAL_PORT 115200   # Beenden: Ctrl+A, K, Y
```

### Deployment (Mac → Pi)

```bash
# Mit Git (empfohlen)
git add -A && git commit -m "..." && git push
ssh rover "cd ~/selection-panel && git pull && sudo systemctl restart selection-panel"

# Alternativ mit rsync (ohne Commit)
rsync -avz --delete \
    --exclude='firmware' --exclude='hardwaretest' --exclude='venv' \
    --exclude='.git' --exclude='__pycache__' \
    . pi@rover:/home/pi/selection-panel/
```

### systemd-Service

```bash
sudo systemctl restart selection-panel  # Neu starten
sudo systemctl status selection-panel   # Status
journalctl -u selection-panel -f        # Live-Logs
```

## Architektur

### Firmware-Schichtenmodell (Abhängigkeiten nur nach unten)

| Schicht | Verzeichnis | Verantwortung |
|---------|-------------|---------------|
| Entry | `main.cpp` | Queue-Erstellung, Task-Start |
| App | `src/app/` | FreeRTOS Tasks (io_task @ 200Hz, serial_task) |
| Logic | `src/logic/` | Entprellung (30ms), Auswahllogik |
| Driver | `src/drivers/` | CD4021B (Taster-Input), 74HC595 (LED-Output) |
| HAL | `src/hal/` | SPI-Bus-Abstraktion mit Mutex |

### Kritische Design-Entscheidungen

- **Gemeinsamer SPI-Bus**: CD4021B (MODE1, 500kHz) und 74HC595 (MODE0, 1MHz) teilen SCK. `SpiGuard` (RAII) stellt korrekten Modus-Wechsel sicher.
- **First-Bit-Problem**: CD4021B gibt Q8 sofort nach Parallel-Load aus, vor dem ersten Takt. Lösung: `digitalRead()` erfasst es vor dem SPI-Transfer in `cd4021.cpp`.
- **Queue-Entkopplung**: io_task blockiert nie; serial_task kann langsam sein. LogEvent-Queue liefert atomare Snapshots.
- **Active-Low Taster**: Pull-up-Widerstände bedeuten 0 = gedrückt. Nutze `activeLow_pressed()` aus `bitops.h`.
- **LED-Refresh**: Wegen geteiltem SPI-Bus wird LED-Zustand jeden Zyklus aufgefrischt, um Glitches zu verhindern.

### FreeRTOS-Timing

- Core 0: Reserviert für WiFi/BLE Stack
- Core 1: Anwendung (IO + Serial Tasks)
- IO-Task Priorität: 5 (hoch, muss 200Hz jitterfrei halten)
- Serial-Task Priorität: 2 (darf drosseln, darf IO nicht blockieren)
- 5ms Zyklus-Budget: ~120µs genutzt (2.4%), bei 100 Tastern ~600µs (12%)

### Server-Architektur

- **aiohttp + asyncio**: Non-blocking I/O für WebSocket und Serial
- **Thread-basierter Serial-Reader**: Behandelt USB-CDC-Fragmentierung mit 50ms Timeout
- **Preempt-Policy**: Neuer Tastendruck stoppt sofort die aktuelle Wiedergabe

## Konfiguration

### Firmware (`firmware/include/config.h`)

```cpp
constexpr uint8_t BTN_COUNT = 10;           // Skalierbar auf 100
constexpr uint8_t LED_COUNT = 10;
constexpr uint32_t IO_PERIOD_MS = 5;        // 200 Hz
constexpr uint32_t DEBOUNCE_MS = 30;
constexpr bool LATCH_SELECTION = true;      // Auswahl bleibt nach Loslassen
constexpr bool SERIAL_PROTOCOL_ONLY = true; // Debug-Logs für Pi deaktivieren
```

### Server (`server/server.py`)

- `PROTOTYPE_MODE = True` für 10 Medien, `False` für 100
- `ESP32_SETS_LED_LOCALLY = True` - ESP32 setzt LED bei Tastendruck selbst
- `SERIAL_PORT` - Stabiler by-id Pfad zum ESP32

## Protokoll-Referenz

**Wichtig:** Alle IDs sind 1-basiert und 3-stellig formatiert (001-100).

**Boot-Sequenz:** Host darf erst nach Empfang von `READY` Befehle senden. Timeout: 5 Sekunden.

### Serial (ESP32 → Pi)
- `READY` - Boot abgeschlossen
- `FW <version>` - Firmware-Version
- `PRESS 001` - Taster gedrückt
- `RELEASE 001` - Taster losgelassen
- `PONG` - Antwort auf PING
- `OK` - Befehl ausgeführt
- `ERROR <msg>` - Fehler

### Serial (Pi → ESP32)
- `LEDSET 001` - One-Hot: nur diese LED an
- `LEDON/LEDOFF 001` - Additiv ein/aus
- `LEDCLR` / `LEDALL` - Alle aus/an
- `PING` / `STATUS` / `VERSION` / `HELP`
- `TEST` / `STOP` - LED-Lauflicht Test starten/stoppen
- `QRESET` - Queue-Statistik zurücksetzen

### WebSocket (Server ↔ Browser)
- Server → Browser: `{"type":"play","id":3}`, `{"type":"stop"}`
- Browser → Server: `{"type":"ended","id":3}`

## Coding-Standards

| Element | Stil | Beispiel |
|---------|------|----------|
| Dateien | snake_case | `spi_bus.cpp` |
| Typen (Struct) | snake_case + `_t` | `log_event_t` |
| Typen (Enum) | snake_case + `_e` | `led_command_e` |
| Funktionen | snake_case | `start_io_task` |
| Klassen | PascalCase | `SpiBus` |
| Private Member | `_` Präfix | `_mtx` |
| Konstanten | UPPER_SNAKE_CASE | `BTN_COUNT` |

**Regeln:**
- Hardware-Zugriff nur über HAL/Drivers-Schichten
- Keine Magic Numbers - nutze `config.h`
- Kein `Serial.print()` im IO-Task-Pfad (Queue zum serial_task nutzen)
- Kein blockierendes `delay()` in Tasks (`vTaskDelayUntil` verwenden)

## Debugging

Ausführliches Logging in `config.h` aktivieren:
```cpp
constexpr bool LOG_VERBOSE_PER_ID = true;
constexpr bool LOG_ON_RAW_CHANGE = true;
constexpr bool SERIAL_PROTOCOL_ONLY = false;
```

STATUS-Ausgabe verstehen:
```
LEDS 0000100000      ← Bit-Vektor (MSB links), LED 5 an
BTNS 1111111111      ← Active-Low: 1 = losgelassen
CURLED 5             ← Aktuelle LED (1-basiert)
HEAP 372756          ← Freier Heap-Speicher
QOVFL 0              ← Queue-Überläufe
MODE PROTOTYPE       ← Betriebsmodus
MAPPING 15,12,...    ← Pin-Mapping (falls konfiguriert)
```

## Skalierung auf 100 Taster

Zwei Konstanten in `config.h` ändern:
```cpp
constexpr uint8_t BTN_COUNT = 100;
constexpr uint8_t LED_COUNT = 100;
```

`BTN_BYTES`/`LED_BYTES` berechnen sich automatisch auf 13. SPI-Transferzeit steigt von ~40µs auf ~320µs (innerhalb des 5ms-Budgets). Server: `PROTOTYPE_MODE = False` setzen.

## Pin-Belegung (XIAO ESP32-S3)

| Pin | GPIO | Funktion | Chip |
|-----|------|----------|------|
| D0 | GPIO1 | LED_RCK (Latch) | 74HC595 |
| D1 | GPIO2 | BTN_PS (Parallel/Shift) | CD4021B |
| D2 | GPIO3 | LED_OE (PWM) | 74HC595 |
| D8 | GPIO7 | SCK (gemeinsamer Takt) | Beide |
| D9 | GPIO8 | BTN_MISO | CD4021B |
| D10 | GPIO9 | LED_MOSI | 74HC595 |

Reserviert: GPIO0 (Boot), GPIO43/44 (USB Serial), GPIO19/20 (USB D+/D-)
