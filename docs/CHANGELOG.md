# CHANGELOG — Änderungshistorie

> Format: [Keep a Changelog](https://keepachangelog.com/de/1.0.0/)

## [2.4.2] — 2025-01-01

### Hinzugefügt

- **Server:** Parallele Ausführung mit `asyncio.gather()` für minimale Latenz
- **Server:** `ESP32_SETS_LED_LOCALLY` Flag für optimierte LED-Steuerung
- **Server:** Health-Endpoint mit Status-Codes (200/503)
- **Dashboard v2.3.0:** Medien-Preloading nach Audio-Unlock
- **Dashboard:** Cache für sofortige Wiedergabe (< 50ms)
- **Dashboard:** Preload-Fortschrittsanzeige
- **Firmware v2.4.1:** LED reagiert sofort bei Tastendruck (< 1ms)
- **Firmware:** CURLED in STATUS-Ausgabe
- **Firmware:** QRESET-Befehl für Queue-Statistik

### Geändert

- Server sendet LEDSET nur wenn ESP32 LED nicht selbst gesetzt hat
- Dashboard nutzt Semaphore für begrenzte Preload-Parallelität
- Dokumentation komplett auf v2.4.2 aktualisiert

### Behoben

- LED-Latenz durch lokale Steuerung im ESP32 eliminiert

---

## [2.4.1] — 2025-12-30

### Hinzugefügt

- **Prototyp funktionsfähig:** Alle 10 Taster erkannt und zugeordnet
- Robuster Parser für fragmentierte Serial-Daten
- TESTBEFEHLE.md in COMMANDS.md integriert

### Geändert

- Server: `os.open` + `stty` statt pyserial (ESP32-S3 USB-CDC kompatibel)
- Medien: 1-basierte Nummerierung (001-010 statt 000-009)
- Dokumentation komplett aktualisiert auf v2.4.1

### Behoben

- Serial-Fragmentierung durch robusten Parser abgefangen

---

## [2.4.0] — 2025-12-30

### Hinzugefügt

- Firmware: `Serial.flush()` nach jedem Event
- Firmware: `snprintf()` für atomare Nachrichten

### Geändert

- Durchgängig 1-basierte Nummerierung (Taster, Serial, Medien, Dashboard)
- Medien umbenannt: 000-009 → 001-010

### Behoben

- ESP32-S3 USB-CDC Fragmentierung durch `Serial.flush()`

---

## [2.3.2] — 2025-12-30

### Geändert

- Server: `os.open` + `select` statt pyserial
- Server: Threading statt serial_asyncio

### Behoben

- pyserial inkompatibel mit ESP32-S3 USB JTAG/serial debug unit

---

## [2.3.1] — 2025-12-30

### Geändert

- Server: Threading-basierte Serial-Lösung statt serial_asyncio

### Behoben

- serial_asyncio: "device reports readiness but returned no data"

---

## [2.3.0] — 2025-12-30

### Hinzugefügt

- **Bit-Mapping** für Prototyp-Verdrahtung (`BUTTON_BIT_MAP`)
- **HELLO-Befehl** zeigt Startup-Nachricht mit Pinbelegung
- 1-basierte Kommunikation (PRESS 001-010)

### Geändert

- Serial-Port auf `/dev/ttyACM0` geändert
- CD4021 Timing: längere Load-Pulse (5 µs)

---

## [2.2.5] — 2025-12-27

### Geändert

- **Dokumentation:** Bloch-Standard angewendet (Fachbegriffe erklärt, Redundanzen entfernt)
- SPEC.md als Single Source of Truth für Glossar
- README.md als Hub ohne Duplikate
- Alle Dokumente ca. 40% kürzer

---

## [2.2.4] — 2025-12-27

### Hinzugefügt

- Präsentation (LaTeX Beamer) mit allen Projektbildern
- CD4021 Timing-Diagramm
- Vollausbau-Blockdiagramm (100×)

### Korrigiert

- Verdrahtungsregeln ergänzt (SER → VCC am letzten IC)

---

## [2.2.3] — 2025-12-26

### Korrigiert

- Pinbelegung D4/D5 statt D6/D7
- ID-Schema vereinheitlicht (später auf 1-basiert geändert)
- SPEC.md als Single Source of Truth

---

## [2.2.0] — 2025-12-25

### Hinzugefügt

- **Phase 5 abgeschlossen:** Web-Dashboard
- Test-Endpoints: `/test/play/{id}`, `/status`, `/health`
- Debug-Panel mit Live-Logs
- Auto-Reconnect bei WebSocket-Abbruch
- Test-Medien-Generator
- Queue-Overflow-Statistik (`QRESET`)

### Geändert

- Server nutzt Python Virtual Environment
- Dashboard mit „Sound aktivieren"-Button
- `PROTOTYPE_MODE` Switch für Skalierung

---

## [2.1.0] — 2025-12-25

### Hinzugefügt

- **Dual-Core FreeRTOS:** Button-Scan auf Core 0, Serial auf Core 1
- **Watchdog:** 5 s Timeout mit Reset
- **Mutex:** Thread-sicherer Hardware-Zugriff
- Befehle: `TEST`, `STOP`, `VERSION`, `STATUS`

### Behoben

- Race-Condition bei schnellen Tastendrücken

---

## [2.0.0] — 2025-12-25

### Geändert

- **CD4021BE** statt 74HC165 (bessere DIP-Verfügbarkeit)
- Separate Pins D0–D2 (Output), D3–D5 (Input)
- Load-Logik invertiert: `P/S = HIGH`

### Entfernt

- 74HC165 Unterstützung

---

## [1.0.0] — 2025-12-22

### Hinzugefügt

- Initiale Projektstruktur
- 74HC595 + 74HC165 Schieberegister
- Serial-Protokoll (`PRESS`, `LEDSET`)
- aiohttp Server mit WebSocket
- Einfaches Dashboard
