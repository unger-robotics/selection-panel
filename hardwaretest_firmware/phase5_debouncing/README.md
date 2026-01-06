# Phase 5: Debouncing (Entprellen)

Zwei Varianten zum Entprellen mechanischer Taster.

## Problem: Prellen

```
Physikalisch:        Ohne Debouncing:
    ─┐ ┌─┐ ┌──       → 5x PRESS erkannt!
     └─┘ └─┘         
     ←─────→
      ~5-20ms
```

## Struktur

```
phase5_debouncing/
├── phase5_simple/      Zeitbasiertes Debouncing
├── phase5_advanced/    State Machine mit Events
└── README.md
```

## Variante 1: Simple (Zeitbasiert)

**Prinzip:** Zustand erst übernehmen, wenn er für 30ms stabil war.

```cpp
if (now - lastChange >= DEBOUNCE_MS) {
    btnDebounced = btnRaw;
}
```

**Vorteile:**
- Einfach zu verstehen
- Wenig Code
- Gut für Prototypen

**Nachteile:**
- Verzögerung bei jedem Tastendruck
- Keine Events (Press/Release)

**Ausgabe:**
```
T 1 PRESSED (debounced) | RAW:01111111 11111111 | DEB:01111111 11111111
```

## Variante 2: Advanced (State Machine)

**Prinzip:** State Machine mit 4 Zuständen und Events.

```
IDLE ──press──► PRESSING ──stable──► PRESSED
  ▲                                     │
  └──────release──◄─ RELEASING ◄────────┘
```

**Vorteile:**
- PRESS Event (einmalig beim Drücken)
- RELEASE Event (einmalig beim Loslassen)
- Toggle-Modus möglich
- Produktionsreif

**Ausgabe:**
```
T 1 PRESS   -> LED ON
T 1 RELEASE
T 1 PRESS   -> LED OFF
T 1 RELEASE
```

## Vergleich

| Feature | Simple | Advanced |
|---------|--------|----------|
| Code-Komplexität | Gering | Mittel |
| Press-Event | Nein | Ja |
| Release-Event | Nein | Ja |
| Toggle-Modus | Nein | Ja |
| Für Prototyp | ✓ | ✓ |
| Für Produktion | – | ✓ |

## Build & Flash

```bash
# Simple
cd phase5_debouncing/phase5_simple
pio run -t upload
pio device monitor

# Advanced
cd phase5_debouncing/phase5_advanced
pio run -t upload
pio device monitor
```

## Konfiguration

In `config.h`:

```cpp
constexpr uint32_t DEBOUNCE_MS = 30;  // Entprellzeit (ms)
constexpr uint32_t LOOP_DELAY_MS = 5; // Schnellere Abtastung
```

## State Machine Diagramm

```
         ┌─────────────────────────────────────────┐
         │                                         │
         ▼                                         │
     ┌──────┐   press    ┌──────────┐   30ms   ┌───────┐
     │ IDLE │ ─────────► │ PRESSING │ ───────► │PRESSED│
     └──────┘            └──────────┘          └───────┘
         ▲                    │                    │
         │                    │ prellen            │ release
         │                    ▼                    ▼
         │               ┌──────┐            ┌──────────┐
         │               │ IDLE │            │RELEASING │
         │               └──────┘            └──────────┘
         │                                         │
         │                    30ms                 │
         └─────────────────────────────────────────┘
```

## Nächste Phase

**Phase 6:** ESP32 ↔ Raspberry Pi 5 Kommunikation
- micro-ROS / ROS 2 Humble
- USB-Serial oder WiFi
- Bidirektionale Steuerung
