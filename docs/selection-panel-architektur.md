# Selection Panel – Systemarchitektur

**Version 2.4.x | Technische Dokumentation**

---

## Überblick: Was wir hier bauen

Stellen wir uns ein Museumspanel vor: 100 beleuchtete Tasten, jede verknüpft mit einem Bild und einer Audiodatei. Ein Besucher drückt Taste 42 – innerhalb von Millisekunden leuchtet die LED, das Bild erscheint auf dem Display, die Audiodatei spielt. Drückt er während der Wiedergabe Taste 17, stoppt alles sofort und Taste 17 übernimmt.

Doch was passiert zwischen dem Tastendruck und dem hörbaren Ton? Ein Blick auf die Architektur zeigt vier Schichten, die nahtlos zusammenarbeiten:

```
┌─────────────────────────────────────────────────────────────────────┐
│  BROWSER (Frontend)                                                 │
│  index.html + app.js + styles.css                                   │
│  - Zeigt Medien an                                                  │
│  - Verwaltet Audio-Wiedergabe                                       │
└─────────────────────────────────────────────────────────────────────┘
                              ▲
                              │ WebSocket (JSON)
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│  RASPBERRY PI (Server)                                              │
│  server.py (Python aiohttp)                                         │
│  - Koordiniert alle Komponenten                                     │
│  - Verwaltet Medien-Dateien                                         │
└─────────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Serial UART (115200 Baud)
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│  ESP32-S3 (Firmware)                                                │
│  main.cpp + shift_register.* + config.h                             │
│  - Scannt 100 Tasten                                                │
│  - Steuert 100 LEDs                                                 │
└─────────────────────────────────────────────────────────────────────┘
                              ▲
                              │ SPI-ähnliches Protokoll
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│  HARDWARE                                                           │
│  74HC595 (Output) + CD4021BE (Input)                                │
│  - Schieberegister für I/O-Erweiterung                              │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Teil 1: Die Hardware-Schicht

### Das Problem: 100 Tasten, aber nur 6 Pins

Der ESP32-S3 XIAO besitzt nur etwa 11 nutzbare GPIO-Pins (*General Purpose Input/Output* – universelle Ein-/Ausgänge). Wir brauchen jedoch 100 Eingänge für Tasten und 100 Ausgänge für LEDs. Wie lösen wir dieses Missverhältnis?

Die Antwort liegt in **Schieberegistern** (*Shift Registers*) – integrierten Schaltkreisen, die serielle Daten in parallele Signale umwandeln und umgekehrt. Mit nur 3 Steuerleitungen kontrollieren wir beliebig viele Ein- oder Ausgänge.

### 74HC595: Der Output-Expander

Betrachten wir zuerst den 74HC595, der unsere LEDs steuert. Dieser IC (*Integrated Circuit* – integrierter Schaltkreis) funktioniert wie eine Eimerkette:

```
Daten-Pin (SER) ──►[Q0]──►[Q1]──►[Q2]──►[Q3]──►[Q4]──►[Q5]──►[Q6]──►[Q7]──► zum nächsten IC
                    │      │      │      │      │      │      │      │
                   LED0   LED1   LED2   LED3   LED4   LED5   LED6   LED7
```

**So funktioniert das Schieben:**

1. Wir legen ein Bit an den Daten-Pin (HIGH oder LOW)
2. Ein Clock-Puls schiebt dieses Bit in Position Q0
3. Das bisherige Q0-Bit wandert nach Q1, Q1 nach Q2, und so weiter
4. Nach 8 Pulsen haben wir alle 8 Bits geladen
5. Ein Latch-Puls (*Latch* = Verriegelung) überträgt die Bits auf die Ausgänge

Die Eleganz liegt in der Kaskadierung: Das „herausfallende" Bit von Q7 speist den Daten-Eingang des nächsten ICs. So ketten wir 13 Chips aneinander und steuern 104 Ausgänge mit denselben 3 Leitungen.

**Die relevanten Timing-Parameter aus dem Datenblatt:**

| Parameter | Minimum | Unsere Einstellung | Sicherheitsfaktor |
|-----------|---------|-------------------|-------------------|
| Clock-Pulsbreite | 20 ns @ 5V | 1 µs | 50× |
| Daten-Setup-Zeit | 25 ns @ 5V | 1 µs | 40× |

Wir arbeiten bei 3,3V, wo die Zeiten länger werden – unsere 1 µs Delays bieten reichlich Reserve.

### CD4021BE: Der Input-Expander

Der CD4021BE arbeitet spiegelbildlich: Er liest parallele Eingänge und gibt sie seriell aus. Hier lauert jedoch eine Falle, die viele Entwickler stolpern lässt.

**Die invertierte Load-Logik:**

| IC | Parallel Load | Serial Shift |
|----|---------------|--------------|
| 74HC165 (üblicher) | SH/LD = LOW | SH/LD = HIGH |
| **CD4021BE** (unser) | P/S = **HIGH** | P/S = **LOW** |

Diese Invertierung führte in frühen Versionen zu schwer auffindbaren Bugs. Die korrekte Sequenz:

```
1. P/S-Pin auf HIGH setzen (Parallel Load – Eingänge einlesen)
2. 2 µs warten (t_WH: Load-Pulsbreite)
3. P/S-Pin auf LOW setzen (Serial Mode)
4. 2 µs warten (t_REM: Zeit bis Q8 stabil)
5. Erstes Bit lesen (liegt SOFORT an Q8!)
6. Clock-Puls → nächstes Bit nach Q8
7. Wiederholen bis alle Bits gelesen
8. WICHTIG: Kein Clock nach dem letzten Bit!
```

Der letzte Punkt ist kritisch: Ein überflüssiger Clock-Puls würde das erste Bit des nächsten Zyklus verschlucken. Diese Erkenntnis stammt aus dem Hardware-Test v1.1.1 und ist im Code dokumentiert.

### Pull-up-Widerstände und Active-Low-Logik

Jeder Taster verbindet seinen Eingang mit GND, wenn gedrückt. Ohne gedrückten Taster würde der Eingang „floaten" – er nimmt zufällige Werte durch elektromagnetische Einstreuung an. Ein **Pull-up-Widerstand** (typisch 10 kΩ) zieht den Eingang auf VCC (3,3V):

```
    VCC (3,3V)
        │
       [10kΩ]  ← Pull-up-Widerstand
        │
        ├──────► zum CD4021-Eingang
        │
      [Taster]
        │
       GND

Taster offen:   Eingang = HIGH (3,3V über Pull-up)
Taster gedrückt: Eingang = LOW (GND über Taster, <1Ω)
```

Diese **Active-Low-Logik** (*aktiv-niedrig*) invertieren wir in der Software, damit `isPressed()` intuitiv `true` zurückgibt, wenn der Taster gedrückt ist.

---

## Teil 2: Die Firmware-Schicht

### Dual-Core-Architektur: Warum zwei Kerne?

Der ESP32-S3 besitzt zwei Prozessorkerne, die wir gezielt einsetzen:

```
┌─────────────────────────────────────────┐
│            CORE 0                       │
│  ┌───────────────────────────────────┐  │
│  │        buttonTask                 │  │
│  │  - Priorität: 2 (hoch)            │  │
│  │  - Scannt alle 10 ms              │  │
│  │  - Debouncing                     │  │
│  │  - Schreibt in Queue              │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
                    │
                    │ buttonEventQueue (16 Events)
                    ▼
┌─────────────────────────────────────────┐
│            CORE 1                       │
│  ┌───────────────────────────────────┐  │
│  │      Arduino loop()               │  │
│  │  - Priorität: 1 (normal)          │  │
│  │  - Serial-Kommunikation           │  │
│  │  - LED-Steuerung                  │  │
│  │  - Events aus Queue lesen         │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

**Warum diese Aufteilung?**

Das Button-Scanning muss deterministisch alle 10 ms erfolgen – unabhängig davon, ob gerade ein Serial-Befehl verarbeitet wird. Wenn beide Tasks auf einem Kern liefen, könnte ein langer Serial-Print das Scanning verzögern und Tastendrücke verschlucken.

**FreeRTOS** (*Free Real-Time Operating System*) ist ein Echtzeitbetriebssystem, das diese Aufteilung ermöglicht. Es verwaltet Tasks, deren Prioritäten und die Kommunikation zwischen ihnen.

### Die Queue: Sichere Kern-zu-Kern-Kommunikation

Eine **Queue** (*Warteschlange*) ist ein Thread-sicherer FIFO-Puffer (*First In, First Out*). Core 0 schreibt Button-Events hinein, Core 1 liest sie heraus:

```c
// Core 0 schreibt (nicht-blockierend):
BaseType_t result = xQueueSend(buttonEventQueue, &event, 0);
if (result != pdTRUE) {
    queueStats.overflowCount++;  // Overflow-Statistik führen
}

// Core 1 liest (nicht-blockierend):
while (xQueueReceive(buttonEventQueue, &event, 0) == pdTRUE) {
    handleButtonEvent(event);
}
```

Die Queue fasst 16 Events. Bei einem Polling-Intervall von 10 ms und maximal einem Event pro Zyklus bedeutet das 160 ms Puffer – mehr als genug, selbst wenn Core 1 kurzzeitig blockiert.

### Debouncing: Das Prellen eliminieren

Mechanische Taster „prellen" (*bounce*) beim Schließen: Der Kontakt schließt und öffnet mehrfach innerhalb von 1–50 ms, bevor er stabil wird. Ohne Gegenmaßnahmen würde ein einzelner Tastendruck Dutzende Events erzeugen.

```
Physischer Tastendruck:
    │
    ▼
    ████████████████████████████████  (gedrückt)
    ─────┐┌──┐┌─┐┌────────────────────  (elektrisches Signal)
         └┘  └┘ └                       ← Prellen (typisch 5-20 ms)
              │
              └─ stabiles Signal

Mit Debouncing (50 ms):
    - Erster Flankenwechsel: Timer starten
    - Alle Änderungen innerhalb 50 ms: ignorieren
    - Nach 50 ms stabiler Zustand: Event auslösen
```

Der Code implementiert dies pro Taste individuell:

```c
if ((now - _lastChangeTime[i]) >= DEBOUNCE_TIME_MS) {
    // Zustandsänderung akzeptieren
    _lastChangeTime[i] = now;
}
```

### Thread-Sicherheit durch Mutex

Ein **Mutex** (*Mutual Exclusion* – gegenseitiger Ausschluss) schützt gemeinsam genutzte Ressourcen. Wenn Core 0 gerade die Schieberegister liest, darf Core 1 nicht gleichzeitig schreiben:

```c
bool OutputShiftRegister::takeMutex(TickType_t timeout) {
    if (_mutex == nullptr) return false;
    return xSemaphoreTake(_mutex, timeout) == pdTRUE;
}

void OutputShiftRegister::write(const uint8_t *data) {
    if (!takeMutex()) return;  // Warten auf exklusiven Zugriff

    // Kritischer Abschnitt: nur ein Task gleichzeitig
    for (uint8_t i = 0; i < _numChips; i++) {
        _buffer[i] = data[i];
    }
    shiftOutData();
    pulseLatch();

    giveMutex();  // Zugriff freigeben
}
```

Ohne Mutex könnten sich die Bits zweier gleichzeitiger Schreiboperationen vermischen – ein schwer zu diagnostizierender Bug.

### Die Latenz-Optimierung v2.4.1

Die ursprüngliche Architektur hatte einen Roundtrip:

```
Tastendruck → ESP32 → Serial → Pi → Serial → ESP32 → LED
             └──────────────── ~50-100 ms ────────────────┘
```

Die Optimierung in v2.4.1 verkürzt den kritischen Pfad:

```
Tastendruck → ESP32 → LED (sofort, < 1 ms)
                   └→ Serial → Pi (parallel, für Audio/Bild)
```

Der Schlüsselcode in `handleButtonEvents()`:

```c
if (event.type == ButtonEventType::PRESSED) {
    ledTest.active = false;           // Laufenden Test abbrechen
    leds.setOnly(event.index);        // LED SOFORT setzen
    currentLedIndex = event.index;    // Zustand merken
}
sendButtonEvent(event);               // Pi parallel informieren
```

Messungen zeigen: Die LED leuchtet innerhalb von 1 ms nach dem Tastendruck – das menschliche Auge nimmt dies als instantan wahr.

---

## Teil 3: Die Server-Schicht

### Asynchrone Programmierung mit asyncio

Der Server muss gleichzeitig:

1. Serial-Daten vom ESP32 lesen
2. WebSocket-Verbindungen zu mehreren Browsern halten
3. HTTP-Anfragen für Medien-Dateien beantworten

**Synchroner Code** würde blockieren: Während wir auf Serial-Daten warten, könnten keine WebSocket-Nachrichten verarbeitet werden. **Asynchrone Programmierung** mit `asyncio` löst dieses Problem.

Das Konzept: Anstatt zu blockieren, „parkt" eine Funktion bei `await` und gibt die Kontrolle ab. Sobald die Daten bereit sind, setzt sie fort:

```python
async def handle_button_press(button_id: int) -> None:
    """Verarbeitet Tastendruck – 1-basierte ID."""
    state.current_id = button_id

    # Beide Aktionen parallel ausführen
    await asyncio.gather(
        state.broadcast({"type": "play", "id": button_id}),  # → Browser
        send_led_if_needed(button_id)                        # → ESP32
    )
```

`asyncio.gather()` startet mehrere Coroutinen parallel und wartet, bis alle fertig sind. So erreichen wir maximale Effizienz.

### Die WebSocket-Verbindung

**WebSocket** ist ein Protokoll für bidirektionale Echtzeitkommunikation zwischen Browser und Server. Im Gegensatz zu HTTP (Request-Response) bleibt die Verbindung offen:

```
Browser                              Server
   │                                   │
   │ ─── HTTP Upgrade Request ───────► │
   │ ◄── HTTP 101 Switching ────────── │
   │                                   │
   │ ◄═══ WebSocket-Verbindung ══════► │
   │      (bidirektional, persistent)  │
   │                                   │
   │ ◄── {"type":"play","id":42} ───── │  (Server → Browser)
   │ ─── {"type":"ended","id":42} ──►  │  (Browser → Server)
```

Der Server verwaltet alle verbundenen Clients in einem Set:

```python
class AppState:
    def __init__(self):
        self.ws_clients: set[web.WebSocketResponse] = set()

    async def broadcast(self, message: dict) -> None:
        """Sendet an ALLE verbundenen Browser."""
        data = json.dumps(message)
        dead_clients = set()

        async def send_to_client(ws):
            try:
                await ws.send_str(data)
            except Exception:
                dead_clients.add(ws)

        await asyncio.gather(*[send_to_client(ws) for ws in self.ws_clients])
        self.ws_clients -= dead_clients  # Tote Verbindungen entfernen
```

### Serial-Kommunikation: Der Threading-Kompromiss

Die Serial-Kommunikation läuft in einem separaten Thread, nicht als asyncio-Task. Warum?

Python's `asyncio` und Serial-I/O harmonieren nicht gut: Die `pyserial`-Bibliothek blockiert, und asynchrone Wrapper wie `aioserial` sind oft instabil. Die robuste Lösung:

```python
def serial_thread():
    """Läuft in separatem Thread, kommuniziert über Queue mit asyncio."""
    while True:
        try:
            line = serial_port.readline()
            if line:
                # Event-Loop-safe in asyncio einspeisen
                asyncio.run_coroutine_threadsafe(
                    handle_serial_line(line.decode()),
                    loop
                )
        except Exception as e:
            logging.error(f"Serial-Fehler: {e}")
            break

# Thread starten (Daemon = beendet sich mit Hauptprogramm)
thread = threading.Thread(target=serial_thread, daemon=True)
thread.start()
```

### Das Serial-Protokoll: Robustes Parsing

ESP32's USB-CDC (*USB Communications Device Class*) fragmentiert manchmal Nachrichten. Statt `PRESS 042` kommt `PRES` gefolgt von `S 042`. Der Parser muss damit umgehen:

```python
async def handle_serial_line(line: str) -> None:
    line = line.strip()

    # Normale Form: "PRESS 001"
    if line.startswith("PRESS "):
        button_id = parse_button_id(line[6:])
        if button_id:
            await handle_button_press(button_id)
            return

    # Fragmentierte Form: " 001" oder "001"
    # Aber NICHT wenn es mit "SE" (von RELEASE) anfängt
    if not line.startswith("SE") and not line.startswith("RELEASE"):
        button_id = parse_button_id(line)
        if button_id:
            logging.debug(f"Fragmentiert erkannt: '{line}' → {button_id}")
            await handle_button_press(button_id)
```

---

## Teil 4: Die Frontend-Schicht

### Browser Autoplay Policy: Das Unlock-Problem

Moderne Browser verbieten automatische Audio-Wiedergabe ohne Benutzerinteraktion. Ein Script darf `audio.play()` erst aufrufen, nachdem der Benutzer geklickt hat. Unsere Lösung:

```javascript
async function unlockAudio() {
    // Methode 1: AudioContext (funktioniert auf iOS)
    const AudioContext = window.AudioContext || window.webkitAudioContext;
    if (AudioContext) {
        const ctx = new AudioContext();
        const buffer = ctx.createBuffer(1, 1, 22050);  // 1 Sample Stille
        const source = ctx.createBufferSource();
        source.buffer = buffer;
        source.connect(ctx.destination);
        source.start(0);

        if (ctx.state === 'suspended') {
            await ctx.resume();
        }
    }

    // Methode 2: HTML Audio Element
    const audio = elements.audio;
    audio.src = 'data:audio/wav;base64,...';  // Stilles WAV
    await audio.play();
    audio.pause();

    state.audioUnlocked = true;

    // Nach Unlock: Medien vorladen
    await preloadAllMedia();
}
```

Der „Sound aktivieren"-Button löst `unlockAudio()` aus – danach darf der Browser Audio abspielen.

### Preloading: Instant Playback erreichen

Ohne Preloading läuft der kritische Pfad:

```
Play-Befehl → HTTP-Request für Audio → Download → Dekodierung → Wiedergabe
              └────────────────── 200-500 ms ──────────────────┘
```

Mit Preloading (v2.3.0):

```
Startup:  Alle 10 Medien vorladen → im Cache speichern
Später:   Play-Befehl → aus Cache abspielen → < 10 ms
```

Die Implementierung nutzt eine **Semaphore** (*Zählsemaphor*) für kontrollierte Parallelität:

```javascript
class Semaphore {
    constructor(max) {
        this.max = max;      // Maximale gleichzeitige Zugriffe
        this.current = 0;    // Aktuelle Anzahl
        this.queue = [];     // Wartende Anfragen
    }

    async acquire() {
        if (this.current < this.max) {
            this.current++;
            return;
        }
        // Warten bis Platz frei wird
        await new Promise(resolve => this.queue.push(resolve));
        this.current++;
    }

    release() {
        this.current--;
        if (this.queue.length > 0) {
            this.queue.shift()();  // Nächsten Wartenden aufwecken
        }
    }
}

// Verwendung: Maximal 3 parallele Downloads
const semaphore = new Semaphore(3);
for (const id of ids) {
    await semaphore.acquire();
    preloadMediaPair(id).finally(() => semaphore.release());
}
```

### Der Medien-Cache

Vorgeladene Medien speichern wir in JavaScript-Objekten:

```javascript
const mediaCache = {
    images: {},  // id → HTMLImageElement
    audio: {}    // id → HTMLAudioElement
};

async function preloadMediaPair(id) {
    const id3 = id.toString().padStart(3, '0');

    // Bild vorladen
    const img = new Image();
    img.src = `/media/${id3}.jpg`;
    await new Promise(resolve => { img.onload = resolve; });
    mediaCache.images[id] = img;

    // Audio vorladen
    const audio = new Audio();
    audio.preload = 'auto';
    audio.src = `/media/${id3}.mp3`;
    await new Promise(resolve => { audio.oncanplaythrough = resolve; });
    mediaCache.audio[id] = audio;
}
```

Bei einem Play-Befehl:

```javascript
function handlePlay(id) {
    const cachedAudio = mediaCache.audio[id];
    if (cachedAudio) {
        cachedAudio.currentTime = 0;  // Zum Anfang spulen
        cachedAudio.play();           // Sofort abspielen
    }
}
```

### CSS Design Tokens: Konsistentes Farbschema

Das Stylesheet definiert **CSS Custom Properties** (*CSS-Variablen*) als zentrale Design-Tokens:

```css
:root {
    /* Arduino-Farbpalette */
    --arduino-teal: #00979D;
    --arduino-teal-dark: #005C5F;
    --arduino-teal-light: #62AEB2;

    /* Raspberry Pi-Farbpalette */
    --raspi-red: #C51A4A;
    --raspi-green: #75A928;

    /* Semantische Farben */
    --accent: var(--arduino-teal);
    --highlight: var(--raspi-red);
    --success: var(--raspi-green);
}
```

Diese Tokens verwendet das gesamte UI:

```css
h1 { color: var(--accent); }
#unlock-btn { background: var(--highlight); }
.status-dot.connected { background: var(--success); }
```

Ändern wir `--arduino-teal`, aktualisiert sich das gesamte Interface konsistent.

---

## Teil 5: Das Protokoll im Detail

### Schicht 1: ESP32 ↔ Raspberry Pi (Serial)

**Physikalisch:** UART (*Universal Asynchronous Receiver/Transmitter*) über USB-CDC, 115200 Baud.

**Nachrichtenformat:** ASCII-Text mit Newline-Terminierung.

| Richtung | Nachricht | Bedeutung |
|----------|-----------|-----------|
| ESP32 → Pi | `PRESS 042` | Taste 42 gedrückt (1-basiert!) |
| ESP32 → Pi | `RELEASE 042` | Taste 42 losgelassen |
| ESP32 → Pi | `READY` | Controller bereit |
| ESP32 → Pi | `PONG` | Antwort auf PING |
| Pi → ESP32 | `LEDSET 042` | LED 42 ein (one-hot: alle anderen aus) |
| Pi → ESP32 | `LEDON 042` | LED 42 zusätzlich einschalten |
| Pi → ESP32 | `LEDOFF 042` | LED 42 ausschalten |
| Pi → ESP32 | `LEDCLR` | Alle LEDs aus |
| Pi → ESP32 | `PING` | Verbindungstest |
| Pi → ESP32 | `STATUS` | Zustandsabfrage |

**Wichtig:** Alle Button- und LED-IDs sind **1-basiert** (001-100), nicht 0-basiert. Diese Konvention macht die Anzeige benutzerfreundlicher.

**Optimierung v2.4.x:** Da der ESP32 die LED selbst setzt (Latenz-Optimierung), sendet der Pi `LEDSET` nur noch, wenn nötig:

```python
ESP32_SETS_LED_LOCALLY = True  # ESP32 v2.4.1+ setzt LED selbst

async def send_led_if_needed(button_id: int) -> None:
    if not ESP32_SETS_LED_LOCALLY:
        await state.send_serial(f"LEDSET {button_id:03d}")
```

### Schicht 2: Server ↔ Browser (WebSocket)

**Physikalisch:** TCP über HTTP-Upgrade, JSON-kodiert.

| Richtung | Nachricht | Bedeutung |
|----------|-----------|-----------|
| Server → Browser | `{"type": "play", "id": 42}` | Medien für ID 42 abspielen |
| Server → Browser | `{"type": "stop"}` | Wiedergabe stoppen |
| Browser → Server | `{"type": "ended", "id": 42}` | Audio für ID 42 beendet |
| Browser → Server | `{"type": "ping"}` | Heartbeat |

**Ablauf eines Tastendrucks:**

```
1. Benutzer drückt Taste 42
2. ESP32 sendet "PRESS 042" an Pi
3. Server parst, extrahiert ID 42
4. Server broadcastet {"type": "play", "id": 42} an alle Browser
5. Browser zeigt Bild 042.jpg, spielt 042.mp3
6. Nach Ende: Browser sendet {"type": "ended", "id": 42}
7. Server empfängt, könnte LED ausschalten (optional)
```

### Fehlerbehandlung

**Serial-Verbindungsverlust:**

- Server versucht alle 5 Sekunden Reconnect
- `state.serial_connected` trackt Status
- Browser zeigt Warnung über fehlende Verbindung

**WebSocket-Verbindungsverlust:**

- Browser versucht alle 5 Sekunden Reconnect
- Visueller Indikator (roter Punkt) zeigt Status
- Preloading übersteht Reconnect (Cache bleibt)

**Fehlende Medien:**

- Server validiert beim Start alle erwarteten Dateien
- Fehlende Dateien werden geloggt
- Browser zeigt Platzhalter „Bild nicht gefunden"

---

## Teil 6: Prototyp- vs. Produktionsmodus

Die `config.h` definiert den Build-Modus:

```c
#define PROTOTYPE_MODE  // Auskommentieren für Produktion

#ifdef PROTOTYPE_MODE
    constexpr uint8_t NUM_SHIFT_REGS = 2;   // 2 ICs = 16 Bits
    constexpr uint8_t NUM_LEDS = 10;
    constexpr uint8_t NUM_BUTTONS = 10;
#else
    constexpr uint8_t NUM_SHIFT_REGS = 13;  // 13 ICs = 104 Bits
    constexpr uint8_t NUM_LEDS = 100;
    constexpr uint8_t NUM_BUTTONS = 100;
#endif
```

### Das Bit-Mapping-Problem

Im Prototyp auf dem Breadboard entspricht die physische Verdrahtung nicht der logischen Reihenfolge. Taste 1 liegt vielleicht auf Bit 15, Taste 2 auf Bit 12:

```c
constexpr uint8_t BUTTON_BIT_MAP[10] = {
    15, 12, 13, 11, 10, 9, 8, 14, 7, 4
};
```

Die `ButtonManager`-Klasse übersetzt:

```c
bool ButtonManager::readRawButton(uint8_t index) const {
    uint8_t bitPos;

    #ifdef PROTOTYPE_MODE
    if (USE_BUTTON_MAPPING && index < NUM_BUTTONS) {
        bitPos = BUTTON_BIT_MAP[index];  // Mapping verwenden
    } else {
        bitPos = index;
    }
    #else
    bitPos = index;  // Produktion: lineare Verdrahtung
    #endif

    return _shiftReg.getInput(bitPos);
}
```

In der Produktion mit professionellem PCB (*Printed Circuit Board* – Leiterplatte) ist die Verdrahtung linear – das Mapping entfällt.

---

## Zusammenfassung: Der Datenfluss

```
TASTENDRUCK (physisch)
        │
        ▼
    CD4021BE liest LOW (Active-Low via Pull-up)
        │
        ▼
    InputShiftRegister::update() sammelt alle Bits
        │
        ▼
    ButtonManager::update() entprellt, erkennt PRESSED
        │
        ▼
    Event in buttonEventQueue (Core 0 → Core 1)
        │
        ├──────────────────────────────────────────┐
        │                                          │
        ▼                                          ▼
    leds.setOnly(index)                    sendButtonEvent()
    LED leuchtet < 1 ms                    Serial: "PRESS 042"
                                                   │
                                                   ▼
                                           server.py empfängt
                                                   │
                                                   ▼
                                           broadcast({"type":"play"})
                                                   │
                                                   ▼
                                           Browser: handlePlay(42)
                                           Bild + Audio aus Cache
                                                   │
                                                   ▼
                                           AUDIO SPIELT (< 50 ms nach Tastendruck)
```

Die gesamte Kette – vom Tastendruck bis zum hörbaren Ton – dauert unter 50 ms. Die Latenz liegt unter der Wahrnehmungsschwelle bzw. fühlt sich für Menschen wie „sofort“ an.
