# Phase 1: ESP32-S3 Threading-Modelle

Vergleich von Arduino (Single-Threaded) und FreeRTOS (Multi-Threaded) auf dem XIAO ESP32-S3.

## Ordnerstruktur

```
phase1_esp32_test/
├── README.md
├── phase1_arduino/
│   ├── platformio.ini
│   └── src/
│       └── main.cpp        # setup() + loop()
└── phase1_freertos/
    ├── platformio.ini
    └── src/
        └── main.cpp        # FreeRTOS Tasks + Queue
```

---

## Build & Flash

```bash
# Arduino-Variante
cd phase1_arduino
pio run -t upload
pio device monitor

# FreeRTOS-Variante
cd phase1_freertos
pio run -t upload
pio device monitor
```

---

## Konzeptvergleich

### Arduino-Modell

```
┌─────────────────────────────────────────┐
│                Core 1                    │
│  setup() → loop() → loop() → loop() ... │
│              ↓                           │
│         taskA()                          │
│         taskB()                          │
│         delay                            │
└─────────────────────────────────────────┘
         Sequentiell, blockierend
```

### FreeRTOS-Modell

```
┌─────────────────────────────────────────┐
│   Core 0          │      Core 1         │
│   USB-CDC         │   Serial-Task (P10) │
│   (TinyUSB)       │   Task A (P1)       │
│                   │   Task B (P1)       │
│         ◄─────────┼────── Queue         │
└───────────────────┴─────────────────────┘
         Parallel, nicht-blockierend
```

---

## Erwartete Ausgabe

### Arduino

```
========================================
Phase 1: Arduino-Modell (Single-Threaded)
========================================

Beide Tasks laufen sequentiell auf Core 1.

Loop laeuft auf Core: 1

[Task A] Counter: 1 | Core: 1
[Task B] Counter: 2 | Core: 1
---
[Task A] Counter: 2 | Core: 1
[Task B] Counter: 4 | Core: 1
---
```

### FreeRTOS

```
========================================
Phase 1: FreeRTOS Multi-Tasking Demo
========================================

Architektur: Producer-Consumer mit Queue
Task A: 600ms | Task B: 400ms | Core: 1

Tasks gestartet.

[Task A] Counter: 1 | Core: 1
[Task B] Counter: 2 | Core: 1
[Task B] Counter: 4 | Core: 1
[Task A] Counter: 2 | Core: 1
[Task B] Counter: 6 | Core: 1
[Task B] Counter: 8 | Core: 1
[Task A] Counter: 3 | Core: 1
```

---

## Architektur-Entscheidungen

### Warum alle Tasks auf Core 1?

Der XIAO ESP32-S3 nutzt **USB-CDC** (natives USB) statt Hardware-UART. Der TinyUSB-Stack läuft auf Core 0 und hat eigene Interrupts/Tasks.

| Versuch | Problem |
|---------|---------|
| Mutex | USB-CDC-Task ignoriert FreeRTOS-Mutex |
| Spinlock | Schützt nur einen Core |
| Critical Section | Multi-Core Race Conditions |
| **Queue + Core 1** | ✓ Funktioniert |

### Warum Message Queue?

```cpp
// Direkt (Race Condition möglich):
Serial.print("[Task A]");  // ← Task B kann hier unterbrechen
Serial.println(counter);

// Mit Queue (sicher):
xQueueSend(logQueue, &msg, 0);  // Thread-sicher
```

| Aspekt | Direkt | Queue |
|--------|--------|-------|
| Thread-Sicherheit | ❌ | ✓ |
| Blockierung | Ja | Nein |
| Pufferung | Nein | 20 Nachrichten |
| Entkopplung | Eng | Lose |

### Warum Serial-Task Priorität 10?

```
Priorität 10: Serial-Task (wird nicht unterbrochen)
Priorität 1:  Task A, Task B (können sich gegenseitig ablösen)
```

Höhere Priorität = Task läuft bevorzugt. Serial-Task gibt nach jedem Print freiwillig ab mit `vTaskDelay(1)`.

---

## FreeRTOS-Konzepte

### vTaskDelay vs delay

```cpp
delay(100);                          // Blockiert CPU komplett
vTaskDelay(pdMS_TO_TICKS(100));      // Gibt CPU frei für andere Tasks
```

### Queue-Operationen

```cpp
// Erstellen
logQueue = xQueueCreate(20, sizeof(LogMessage));

// Senden (nicht-blockierend)
xQueueSend(logQueue, &msg, 0);

// Empfangen (blockierend bis Nachricht da)
xQueueReceive(logQueue, &msg, portMAX_DELAY);
```

### Task erstellen

```cpp
xTaskCreatePinnedToCore(
    taskFunction,  // Funktion
    "TaskName",    // Name (Debug)
    4096,          // Stack (Bytes)
    NULL,          // Parameter
    1,             // Priorität
    NULL,          // Handle
    1              // Core (0 oder 1)
);
```

---

## Vergleichstabelle

| Aspekt | Arduino | FreeRTOS |
|--------|---------|----------|
| Ausführung | Sequentiell | Präemptiv |
| Timing | Gekoppelt | Unabhängig |
| Blocking | delay() blockiert alles | vTaskDelay() gibt CPU frei |
| Shared Resources | Kein Schutz nötig | Queue/Mutex nötig |
| Komplexität | Niedrig | Mittel |
| Debugging | Einfach | Schwieriger |

---

## Lessons Learned

1. **ESP32-S3 USB-CDC ist speziell** – Core 0 für System-Tasks freilassen
2. **Queue > Mutex** für Serial-Ausgabe mit mehreren Tasks
3. **Prioritäten nutzen** – Serial-Task braucht höchste Priorität
4. **vTaskDelay(1) nach Serial** – USB-Stack braucht CPU-Zeit

---

## Thread – Erklärung

**Thread** = Ausführungsstrang (deutsch: "Faden")

Ein Thread ist eine Sequenz von Anweisungen, die der Prozessor abarbeitet.

---

### Single-Threaded (Ein Faden)

```
Zeit →
──────────────────────────────────────────────►
  [A] → [B] → [A] → [B] → [A] → [B]

  Alles nacheinander, ein Ausführungsstrang
```

**Beispiel:** Arduino `loop()` – eine Aufgabe nach der anderen.

---

### Multi-Threaded (Mehrere Fäden)

```
Zeit →
──────────────────────────────────────────────►
Thread 1:  [A]     [A]     [A]     [A]
Thread 2:     [B]     [B]     [B]     [B]

  Mehrere Ausführungsstränge, quasi-parallel
```

**Beispiel:** FreeRTOS Tasks – mehrere "Programme" laufen gleichzeitig.

---

### Analogie

| Konzept | Analogie |
|---------|----------|
| Single-Threaded | Ein Koch macht alles nacheinander |
| Multi-Threaded | Mehrere Köche arbeiten parallel |
| Core | Küche/Arbeitsplatz |
| Thread | Koch |
| Mutex | "Besetzt"-Schild am Herd |

---

### ESP32-S3 Kontext

| Begriff | Bedeutung |
|---------|-----------|
| Dual-Core | 2 Prozessorkerne (2 Küchen) |
| Thread/Task | Unabhängiger Ausführungsstrang |
| Single-Threaded | `loop()` läuft allein |
| Multi-Threaded | Mehrere FreeRTOS Tasks |
