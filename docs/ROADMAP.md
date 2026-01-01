# ROADMAP — Implementierungsplan

> Phasen und Status. Details: siehe [SPEC.md](SPEC.md), Tests: siehe [RUNBOOK.md](RUNBOOK.md)

| Version | 2.4.2 |
|---------|-------|
| Stand | 2025-01-01 |

---

## Übersicht

```
Phase 1: Infrastruktur        ✅
Phase 2: Hardware-Prototyp    ✅
Phase 3: ESP32 Firmware       ✅
Phase 4: Raspberry Pi Server  ✅
Phase 5: Web-Dashboard        ✅
Phase 6: Integration & Test   ✅ Prototyp (10×)
Phase 7: Skalierung (100×)    ◀ nächste Phase
Phase 8: Produktivbetrieb     ◯
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
| 2× CD4021BE kaskadiert | ✅ |
| 2× 74HC595 kaskadiert | ✅ |
| 10 Taster verdrahtet | ✅ |
| 10 LEDs verdrahtet | ✅ |
| Bit-Mapping implementiert | ✅ |

### Software

| Aufgabe | Status |
|---------|--------|
| Firmware v2.4.1 (LED sofort, CURLED) | ✅ |
| Server v2.4.2 (asyncio.gather) | ✅ |
| Dashboard v2.3.0 (Preloading) | ✅ |
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

## Phase 7: Skalierung (100×) ◀ nächste Phase

### Hardware

| Aufgabe | Status |
|---------|--------|
| 13× CD4021BE kaskadieren | ◯ |
| 13× 74HC595 kaskadieren | ◯ |
| 100 Taster verdrahten | ◯ |
| 100 LEDs verdrahten | ◯ |
| PCB-Design (KiCad) | ◯ |
| Lineare Verdrahtung | ◯ |

### Software

| Aufgabe | Status |
|---------|--------|
| `PROTOTYPE_MODE = False` | ◯ |
| Bit-Mapping entfernen | ◯ |
| 100 Medien-Sets | ◯ |

### Tests

| Test | Status |
|------|--------|
| Hardware End-to-End | ◯ |
| Stresstest (200×) | ◯ |
| Alle 100 Taster | ◯ |

**Meilenstein M7:** System mit 100 Tastern funktionsfähig.

---

## Phase 8: Produktivbetrieb ◯

| Aufgabe | Status |
|---------|--------|
| systemd-Service aktiviert | ◯ |
| Reboot-Test | ◯ |
| 24h Dauertest | ◯ |
| Gehäuse | ◯ |
| Dokumentation finalisiert | ◯ |

**Meilenstein M8:** System läuft 24 h ohne Eingriff.

---

## Gelöste Probleme

| Problem | Lösung | Version |
|---------|--------|---------|
| LED-Latenz durch Roundtrip | ESP32 setzt LED lokal (< 1ms) | FW 2.4.1 |
| Dashboard-Latenz | Medien-Preloading + Cache | Dashboard 2.3.0 |
| Sequentielle Server-Aktionen | `asyncio.gather()` für Parallelität | Server 2.4.2 |
| iOS Audio-Unlock fehlgeschlagen | AudioContext API + Fallback | Dashboard 2.2.5 |
| ESP32-S3 USB-CDC fragmentiert | `Serial.flush()` nach jedem Event | FW 2.4.0 |
| pyserial funktioniert nicht | `os.open` + `stty` statt pyserial | Server 2.4.1 |
| Fragmentierte Serial-Daten | Robuster Parser | Server 2.4.1 |
| Falsche Taster-Zuordnung | Bit-Mapping in config.h | FW 2.3.0 |
| CD4021 Timing-Probleme | Längere Load-Pulse (2µs) | FW 2.3.1 |

---

## Zeitschätzung

| Phase | Aufwand | Status |
|-------|---------|--------|
| 1–5 | 6–9 Tage | ✅ |
| 6 | 3–4 Tage | ✅ |
| 7 | 2–3 Tage | ◯ |
| 8 | 0,5 Tage | ◯ |

---

## Nächste Schritte

1. [ ] PCB-Design für 100 Buttons (KiCad)
2. [ ] Lineare Verdrahtung (kein Bit-Mapping nötig)
3. [ ] 100 Medien-Sets erstellen
4. [ ] End-to-End Tests (100×)
5. [ ] Gehäuse-Design
6. [ ] 24h Dauertest

---

## Versionen

| Komponente | Prototyp | Produktion |
|------------|----------|------------|
| Firmware | 2.4.1 | 2.5.0 (geplant) |
| Server | 2.4.2 | 2.5.0 (geplant) |
| Dashboard | 2.3.0 | 2.4.0 (geplant) |
| Hardware | 2× ICs | 26× ICs |
| Taster | 10 | 100 |
| Medien | 001–010 | 001–100 |
