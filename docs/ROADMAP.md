# ROADMAP — Implementierungsplan

> Phasen und Status. Details: siehe [SPEC.md](SPEC.md), Tests: siehe [RUNBOOK.md](RUNBOOK.md)

| Version | 2.2.5 |
|---------|-------|

---

## Übersicht

```
Phase 1: Infrastruktur        ✅
Phase 2: Hardware-Prototyp    ✅
Phase 3: ESP32 Firmware       ✅
Phase 4: Raspberry Pi Server  ✅
Phase 5: Web-Dashboard        ✅
Phase 6: Integration & Test   ◀ aktiv
Phase 7: Produktivbetrieb     ◯
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

## Phase 6: Integration ◀ aktiv

### Hardware skalieren

| Aufgabe | Status |
|---------|--------|
| 13× CD4021BE kaskadieren | ◯ |
| 13× 74HC595 kaskadieren | ◯ |
| Durchgangsprüfung | ◯ |

### Medien

| Aufgabe | Status |
|---------|--------|
| Test-Medien (000–009) | ✅ |
| Produktiv-Medien (000–099) | ◯ |

### Akzeptanztests

| Test | Status |
|------|--------|
| Preempt (curl) | ✅ |
| Ende → LEDCLR | ✅ |
| Hardware End-to-End | ◯ |
| Stresstest (200×) | ◯ |

**Meilenstein M6:** Alle Tests bestanden.

---

## Phase 7: Produktivbetrieb ◯

| Aufgabe | Status |
|---------|--------|
| systemd-Service aktiviert | ✅ |
| Reboot-Test | ◯ |
| 24h Dauertest | ◯ |

**Meilenstein M7:** System läuft 24 h ohne Eingriff.

---

## Zeitschätzung

| Phase | Aufwand | Status |
|-------|---------|--------|
| 1–5 | 6–9 Tage | ✅ |
| 6 | 2–3 Tage | ◀ aktiv |
| 7 | 0,5 Tage | ◯ |

---

## Nächste Schritte

1. Hardware auf 100× skalieren
2. 100 Medien-Sets erstellen
3. End-to-End Tests
4. 24h Dauertest
