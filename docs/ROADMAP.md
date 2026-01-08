# ROADMAP — Implementierungsplan

> Phasen und Status. Details: siehe [SPEC.md](SPEC.md), Tests: siehe [RUNBOOK.md](RUNBOOK.md)

| Version | 2.5.2 |
|---------|-------|
| Stand | 2026-01-08 |

---

## Übersicht

```
Phase 1: Infrastruktur        ✅
Phase 2: Hardware-Prototyp    ✅
Phase 3: ESP32 Firmware       ✅
Phase 4: Raspberry Pi Server  ✅
Phase 5: Web-Dashboard        ✅
Phase 6: Integration & Test   ✅ Prototyp (10×)
Phase 7: Pi-Integration       ✅ Serial + WebSocket
Phase 8: Skalierung (100×)    ◀ nächste Phase
Phase 9: Produktivbetrieb     ◯
```

---

## Phase 1–5: Abgeschlossen ✅

| Phase | Meilenstein | Status |
|-------|-------------|--------|
| 1 | Toolchain funktioniert | ✅ |
| 2 | Taster → LED reagiert | ✅ |
| 3 | `PRESS` → `LEDSET` funktioniert | ✅ |
| 4 | WebSocket-Verbindung steht | ✅ |
| 5 | Browser zeigt Bild + Audio | ✅ |

---

## Phase 6: Integration (Prototyp) ✅

### Hardware Prototyp

| Aufgabe | Status |
|---------|--------|
| 2× CD4021B kaskadiert | ✅ |
| 2× 74HC595 kaskadiert | ✅ |
| 10 Taster verdrahtet | ✅ |
| 10 LEDs verdrahtet | ✅ |
| Bit-Mapping implementiert | ✅ |

### Software

| Aufgabe | Status |
|---------|--------|
| Firmware v2.5.2 (FreeRTOS, Hardware-SPI) | ✅ |
| Server v2.5.2 (asyncio, by-id Pfad) | ✅ |
| Dashboard v2.5.1 (Preloading) | ✅ |
| 1-basierte Nummerierung | ✅ |

### Medien

| Aufgabe | Status |
|---------|--------|
| Test-Medien (001–010) | ✅ |
| Medien-Validierung | ✅ |
| Preloading im Browser | ✅ |

### Tests

| Test | Status |
|------|--------|
| Alle 10 Taster erkannt | ✅ |
| Zuordnung Taster → Medien | ✅ |
| Preempt (curl) | ✅ |
| Ende → LEDCLR | ✅ |
| WebSocket Broadcast | ✅ |
| LED-Latenz < 1ms | ✅ |
| Dashboard-Latenz < 50ms | ✅ |

**Meilenstein M6:** ✅ Prototyp mit 10 Tastern voll funktionsfähig.

---

## Phase 7: Pi-Integration ✅

### Architektur

| Aufgabe | Status |
|---------|--------|
| ESP32 als reiner I/O-Controller | ✅ |
| Pi als Anwendungslogik-Host | ✅ |
| Serial-Protokoll dokumentiert | ✅ |
| WebSocket-Protokoll dokumentiert | ✅ |
| by-id Serial-Pfad (stabil) | ✅ |

### Firmware

| Aufgabe | Status |
|---------|--------|
| FreeRTOS Dual-Core | ✅ |
| Hardware-SPI (shared Bus) | ✅ |
| First-Bit-Rescue (CD4021B) | ✅ |
| SPI-Modi dokumentiert | ✅ |
| Zeitbasiertes Debouncing | ✅ |

### Server

| Aufgabe | Status |
|---------|--------|
| aiohttp WebSocket | ✅ |
| Serial-Thread mit asyncio Bridge | ✅ |
| Medien-Validierung | ✅ |
| Health-Check Endpoint | ✅ |
| systemd-Service | ✅ |

### Dokumentation

| Aufgabe | Status |
|---------|--------|
| PI-INTEGRATION.md | ✅ |
| PROTOCOL.md | ✅ |
| SPEC.md aktualisiert | ✅ |

**Meilenstein M7:** ✅ Pi-Integration mit Serial + WebSocket funktionsfähig.

---

## Phase 8: Skalierung (100×) ◀ nächste Phase

### Hardware

| Aufgabe | Status |
|---------|--------|
| 13× CD4021B kaskadieren | ◯ |
| 13× 74HC595 kaskadieren | ◯ |
| 100 Taster verdrahten | ◯ |
| 100 LEDs verdrahten | ◯ |
| PCB-Design (KiCad) | ◯ |
| Lineare Verdrahtung | ◯ |

### Software

| Aufgabe | Status |
|---------|--------|
| `PROTOTYPE_MODE = False` | ◯ |
| BTN_COUNT = 100, LED_COUNT = 100 | ◯ |
| 100 Medien-Sets | ◯ |

### Tests

| Test | Status |
|------|--------|
| Hardware End-to-End | ◯ |
| Stresstest | ◯ |
| Alle 100 Taster | ◯ |

**Meilenstein M8:** System mit 100 Tastern funktionsfähig.

---

## Phase 9: Produktivbetrieb ◯

| Aufgabe | Status |
|---------|--------|
| systemd-Service aktiviert | ◯ |
| Reboot-Test | ◯ |
| Dauertest | ◯ |
| Gehäuse | ◯ |
| Dokumentation finalisiert | ◯ |

**Meilenstein M9:** System läuft.

---

## Gelöste Probleme

| Problem | Lösung | Version |
|---------|--------|---------|
| LED-Latenz durch Roundtrip | ESP32 setzt LED lokal (< 1ms) | FW 2.5.2 |
| Dashboard-Latenz | Medien-Preloading + Cache | Dashboard 2.5.1 |
| Sequentielle Server-Aktionen | `asyncio.gather()` für Parallelität | Server 2.5.2 |
| iOS Audio-Unlock fehlgeschlagen | AudioContext API + Fallback | Dashboard 2.5.1 |
| ESP32-S3 USB-CDC fragmentiert | `Serial.flush()` nach jedem Event | FW 2.5.2 |
| pyserial funktioniert nicht | `os.open` + `stty` statt pyserial | Server 2.5.2 |
| CD4021B First-Bit-Problem | digitalRead() vor SPI | FW 2.5.2 |
| CD4021B Timing-Probleme | Längere Load-Pulse (2µs) | FW 2.5.2 |
| Serial-Pfad instabil | by-id Pfad verwenden | Server 2.5.2 |

---

## Zeitschätzung

| Phase | Aufwand | Status |
|-------|---------|--------|
| 1–5 | 6–9 Tage | ✅ |
| 6 | 3–4 Tage | ✅ |
| 7 | 2–3 Tage | ✅ |
| 8 | 2–3 Tage | ◯ |
| 9 | 0,5 Tage | ◯ |

---

## Nächste Schritte

1. [ ] Schaltplan (KiCad)
2. [ ] Lineare Verdrahtung (kein Bit-Mapping nötig)
3. [ ] 100 Medien-Sets erstellen
4. [ ] End-to-End Tests (100×)
5. [ ] Gehäuse-Design

---

## Versionen

| Komponente | Prototyp (aktuell) | Produktion (geplant) |
|------------|----------|------------|
| Firmware | 2.5.2 | 3.0.0 |
| Server | 2.5.2 | 3.0.0 |
| Dashboard | 2.5.1 | 3.0.0 |
| Hardware | 2× ICs | 26× ICs |
| Taster | 10 | 100 |
| Medien | 001–010 | 001–100 |

---

*Stand: 2026-01-08 | Version 2.5.2*
