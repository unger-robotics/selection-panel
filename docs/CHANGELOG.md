# CHANGELOG — Änderungshistorie

> Format: [Keep a Changelog](https://keepachangelog.com/de/1.0.0/)

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
- ID-Schema vereinheitlicht auf 0-basiert (000–099)
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
