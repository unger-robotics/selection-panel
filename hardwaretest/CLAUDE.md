# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Übersicht

Hardware-Test-Firmware-Sammlung für das Selection Panel Projekt. Progressive Phasen validieren jede Hardware-Komponente vor der Integration: ESP32-S3 → LEDs → Taster → Kombiniert → FreeRTOS → Modulare Architektur.

**Ziel-Hardware:** Seeed XIAO ESP32-S3 mit CD4021B (Taster-Eingänge) und 74HC595 (LED-Ausgänge) Schieberegistern, die sich einen gemeinsamen SPI-Takt teilen.

## Build-Befehle

Alle Phasen nutzen PlatformIO. Zuerst ins jeweilige Phasen-Verzeichnis wechseln:

```bash
cd phase6_modular                # Oder beliebiges phase1-7 Verzeichnis
pio run -t upload                # Kompilieren und flashen
pio device monitor               # Serial-Monitor (115200 Baud)
pio run -t upload -t monitor     # Build + Flash + Monitor in einem Schritt
pio run -t clean                 # Build-Artefakte löschen
```

## Phasen-Übersicht

| Phase | Zweck | Kernerkenntnisse |
|-------|-------|------------------|
| 1 | ESP32 Grundlagen | Arduino-Loop vs. FreeRTOS-Tasks |
| 2 | LED-Ausgabe | 74HC595 Daisy-Chain, LSB-first, Ghost-LED-Maskierung |
| 3 | Taster-Eingabe | CD4021B First-Bit-Problem, MSB-first, Active-Low |
| 4 | Kombiniertes SPI | SPI-Modus-Wechsel (MODE0/MODE1 auf geteiltem Bus) |
| 5 | FreeRTOS | Queue-basierte Task-Entkopplung, kein Serial im IO-Pfad |
| 6 | Modular | Zielarchitektur: HAL → Drivers → Logic → App |
| 7 | Pi-Integration | Serial-Protokoll-Bridge |

**Phase 6 ist die Referenz-Implementierung** – als Vorlage für Produktions-Firmware verwenden.

## Architektur (Phase 6)

```
main.cpp           Entry: Queue-Erstellung, Task-Start
    ↓
app/               FreeRTOS-Tasks (io_task @ 200Hz, serial_task)
    ↓
logic/             Geschäftslogik (Entprellung, One-Hot-Auswahl)
    ↓
drivers/           Hardware-Treiber (cd4021, hc595)
    ↓
hal/               SPI-Bus-Abstraktion mit FreeRTOS-Mutex
```

Abhängigkeiten fließen nur nach unten. Keine Schicht darf von einer darüberliegenden importieren.

## Kritische Hardware-Details

**Geteilter SPI-Bus:**
- CD4021B nutzt MODE1 @ 500kHz (Abtastung bei fallender Flanke)
- 74HC595 nutzt MODE0 @ 1MHz (Abtastung bei steigender Flanke)
- `SpiGuard` RAII-Klasse stellt korrekten Modus-Wechsel und Mutex-Schutz sicher

**First-Bit-Problem (CD4021B):**
Der CD4021B gibt Q8 sofort nach Parallel-Load aus, vor der ersten Taktflanke. Lösung: Erstes Bit mit `digitalRead()` vor dem SPI-Transfer erfassen.

**Bit-Reihenfolge:**
- Taster (CD4021B): MSB-first, Active-Low (0 = gedrückt)
- LEDs (74HC595): LSB-first, Active-High (1 = an)

**Latch-Glitch-Vermeidung:**
10kΩ Pull-Down am RCK (Latch) Pin verhindert sporadisches Latchen während Taster-Reads.

## Konfiguration

Alle einstellbaren Parameter in `phase6_modular/include/config.h`:

```cpp
constexpr uint8_t BTN_COUNT = 10;       // Skalierbar auf 100
constexpr uint8_t LED_COUNT = 10;       // Skalierbar auf 100
constexpr uint32_t IO_PERIOD_MS = 5;    // 200 Hz Abtastrate
constexpr uint32_t DEBOUNCE_MS = 30;    // Entprellschwelle
constexpr bool LATCH_SELECTION = true;  // LED bleibt nach Loslassen an
```

## Coding-Regeln

**Dogma:**
- Keine dynamische Speicherallokation zur Laufzeit (FreeRTOS-Queues/Tasks nur beim Start)
- Kein `Serial.print()` im IO-Task-Pfad (Queue zum serial_task nutzen)
- Kein blockierendes `delay()` in Tasks (`vTaskDelayUntil` verwenden)

**Schichtentrennung:**
- Hardware-Zugriff nur über HAL/Drivers-Schichten
- Logik (Debounce/Selection) ohne direkte Pin-/SPI-Zugriffe
- Bit-Manipulationslogik zentral in `bitops.h`

**Code Style:**
- Kommentare erklären **warum** (Timing, Edge-Cases, Hardware-Eigenheiten)
- Kein Copy&Paste von Mapping-/Bitlogik
