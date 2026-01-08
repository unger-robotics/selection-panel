# Python-Code-Guide (am Beispiel `server.py` v2.5.2)

Version 2.5.2 | Stand: 2026-01-08

## 1) Architektur in einem Satz

Ein **aiohttp-Server** bridged **ESP32-Serial → asyncio** und broadcastet Events per **WebSocket** an Browser-Clients; bei jedem Tastendruck gilt **"Umschalten gewinnt"** (Preempt) und **One-Hot-LED**.

## 2) Bauplan: Schichten & Verantwortlichkeiten

**Regel (Bloch-Kernidee):** Jede Komponente soll *eine* Hauptverantwortung haben, mit klaren Schnittstellen.

**Im Code:**

- **Konfiguration (Konstanten):** Ports, Pfade, Mode, Timeouts (`FRAGMENT_TIMEOUT_MS = 50`, Reconnect 5 s).
- **Zustand (State-Objekt):** `AppState` hält `current_id`, WebSocket-Clients, Serial-FD, Media-Status.
- **Serial-Pfad:** `serial_reader_task()` startet einen Thread, der Bytes liest, Zeilen/Fragmente zusammensetzt und per `asyncio.run_coroutine_threadsafe(...)` in den Event-Loop übergibt.
- **Event-Logik:** `handle_button_press()` setzt `current_id`, sendet `stop` und direkt danach `play`.
- **HTTP/WebSocket-API:** `/ws`, `/status`, `/health`, `/test/play/{id}`, `/test/stop`, plus Static/Media-Serving.

**Anwendung (Wie du erweiterst):**

Neue Funktionalität immer dort hinzufügen, wo die Verantwortung liegt:

- neues Serial-Kommando → `handle_serial_line()`
- neue WebSocket-Message → `handle_ws_message()`
- neue HTTP-Route → `create_app()` + eigener Handler

## 3) asyncio-Grundmuster (sauber & robust)

**Regel:** Im Event-Loop keine Blocker (kein `time.sleep`, kein blocking IO). Blockendes IO → Thread/Process; Ergebnisse → zurück in den Loop.

**Im Code:**

- Serial-IO ist bewusst im Thread (poll + nonblocking FD).
- Event-Loop übernimmt nur: Parsing-Resultate verarbeiten, broadcasten, (optional) Serial-TX.

**Praxis-Pattern:**

- Parallelität im Loop: `await asyncio.gather(...)` für gleichzeitige Sends.
- Broadcast resilient: fehlerhafte Clients sammeln und aus `ws_clients` entfernen.

## 4) Serial-Parsing: Fragmentierung korrekt behandeln

**Beobachtung → Daten → Regel (induktiv):**

- Beobachtung: USB-CDC kann `PRESS` und `003` getrennt liefern.
- Daten: Pending-Fragment + Timeout-Vervollständigung (50 ms).
- Regel: **Bytes puffern → Zeilen bilden → ggf. Fragmente kombinieren → erst dann "Event" erzeugen.**

**Anwendung (wenn du neue Befehle hinzufügst):**

Erweitere `handle_serial_line()` um **eindeutige Prefixe** (z. B. `SENSOR`, `ACK`), damit Fragmente nicht "aus Versehen" als Zahlen-ID interpretiert werden. Halte `parse_button_id()` strikt (nur `isdigit`, Range-Check).

## 5) Preempt & One-Hot: Race-Conditions kontrollieren

**Regel:** Bei konkurrierenden Events entscheidet eine **monotone Wahrheit** (hier: `state.current_id`). Alles, was "später" reinkommt, muss gegen diese Wahrheit geprüft werden.

**Im Code:**

- Preempt: neuer Tastendruck setzt sofort `state.current_id` und sendet `stop` + `play`.
- Playback-Ende: `handle_playback_ended()` löscht LEDs nur, wenn `ended_id == current_id` (Race-Schutz).

**Anwendung (wenn du Logik verschärfen willst):**

Ergänze optional eine **laufende Sequenznummer** (z. B. `event_seq += 1`) und sende sie mit `play`; dann kann der Browser "ended" ebenfalls eindeutig zuordnen (hilft bei Multi-Tab/Latency).

## 6) Medien-Validierung: Start-Checks und Runtime-Checks

**Regel:** Teure Validierung früh (Startup), schnelle Checks zur Laufzeit.

**Im Code:**

- Startup: `validate_media()` prüft pro ID `.jpg` und `.mp3`, zählt, loggt fehlende Dateien.
- Runtime: `check_media_exists()` liefert Status für eine ID.

**Anwendung (Best-Practice):**

`/health` ist "degraded" wenn Serial down oder Medien fehlen; gut für systemd/Monitoring. Für Produktion: fehlende Medien optional als **harte Startbedingung** (Exitcode ≠ 0), wenn du "fail fast" willst.

## 7) Code-Qualität: konkrete Leitplanken

**Regeln, die sich in der Praxis auszahlen:**

1. **Typen durchziehen:** du nutzt bereits `Optional[int]`, Return-Types; halte das konsequent (auch für WS-Payload-Schemas).
2. **Logging statt Print:** bereits sauber (`LOG_FORMAT`, Levels). Für Debug-Phasen: gezielte `logging.debug` in Parser/State-Übergängen.
3. **Konstanten zentral:** Mode/Anzahl Medien über `PROTOTYPE_MODE` und `NUM_MEDIA`.
4. **Schnittstellen schmal halten:** `handle_button_press(button_id)` ist der zentrale Eingang für "User-Intent".

## 8) Protokoll-Übersicht

### Serial-Protokoll (ESP32 ↔ Pi)

| Richtung | Nachricht | Bedeutung |
|----------|-----------|-----------|
| ESP32 → Pi | `READY` | ESP32 bereit |
| ESP32 → Pi | `FW SelectionPanel v2.5.2` | Firmware-Version |
| ESP32 → Pi | `PRESS 001` | Taster 1 gedrückt |
| ESP32 → Pi | `RELEASE 001` | Taster 1 losgelassen |
| ESP32 → Pi | `PONG` | Antwort auf PING |
| Pi → ESP32 | `LEDSET 001` | LED 1 an (One-Hot) |
| Pi → ESP32 | `LEDCLR` | Alle LEDs aus |
| Pi → ESP32 | `PING` | Verbindungstest |

### WebSocket-Protokoll (Server ↔ Browser)

| Richtung | Message | Beschreibung |
|----------|---------|--------------|
| Server → Browser | `{"type":"play","id":n}` | Wiedergabe starten |
| Server → Browser | `{"type":"stop"}` | Wiedergabe stoppen |
| Browser → Server | `{"type":"ended","id":n}` | Audio beendet |
| Browser → Server | `{"type":"ping"}` | Heartbeat |

### HTTP-Endpoints

| Endpoint | Methode | Beschreibung |
|----------|---------|--------------|
| `/` | GET | Web-Dashboard |
| `/ws` | WebSocket | Echtzeit-Events |
| `/static/` | GET | JavaScript, CSS |
| `/media/` | GET | Bilder und Audio |
| `/status` | GET | Server-Status (JSON) |
| `/health` | GET | Health-Check (200/503) |
| `/test/play/{id}` | GET | Tastendruck simulieren |
| `/test/stop` | GET | Wiedergabe stoppen |

## Glossar (alphabetisch)

- **aiohttp** – Python-Webframework für HTTP-Server und WebSockets auf asyncio-Basis.
- **async / await** – Syntax, um asynchrone Funktionen (Coroutines) nicht-blockierend auszuführen.
- **asyncio** – Standardbibliothek für kooperatives Multitasking (Event-Loop, Tasks, Futures).
- **broadcast** – Senden derselben Nachricht an mehrere Empfänger (hier: alle WebSocket-Clients).
- **Coroutine** – Asynchrone Funktion, die vom Event-Loop ausgeführt und pausiert/fortgesetzt werden kann.
- **Daemon-Thread** – Thread, der das Programm nicht am Beenden hindert (läuft "im Hintergrund").
- **Event-Loop** – Zentrale Ausführungsschleife, die Coroutines plant und IO-Ereignisse verarbeitet.
- **File Descriptor (FD)** – Integer-Handle eines geöffneten OS-Ressourcenkanals (Datei, Serial-Device).
- **Fragmentierung** – Aufteilung logisch zusammengehöriger Daten in mehrere Pakete/Chunks (z. B. USB-CDC).
- **gather** – asyncio-Funktion, die mehrere awaitables parallel startet und gemeinsam abwartet.
- **Health-Check** – Endpoint, der "gesund/degraded" für Monitoring/Orchestrierung ausgibt (z. B. systemd).
- **Idempotent** – Mehrfaches Ausführen führt zum selben Ergebnis (wichtig bei Retries/Startup-Hooks).
- **JSON** – Textformat für strukturierte Daten (Key-Value), oft für WebSocket-Payloads genutzt.
- **Lock (Mutex)** – Synchronisationsobjekt, das kritische Abschnitte gegen gleichzeitigen Zugriff schützt.
- **non-blocking IO** – Lese/Schreiboperationen blockieren nicht; statt warten gibt es "kein Datum verfügbar".
- **One-Hot** – Kodierung, bei der genau ein Bit/Element "1/aktiv" ist, alle anderen "0/inaktiv".
- **poll** – Systemcall/Mechanismus, um auf IO-Events mehrerer FDs zu warten.
- **Preempt** – "Neues Ereignis gewinnt": laufende Aktion wird sofort verdrängt/überschrieben.
- **Race-Condition** – Timing-abhängiger Fehler, wenn konkurrierende Abläufe in ungünstiger Reihenfolge wirken.
- **Reconnect-Loop** – Wiederholtes Verbinden nach Fehler/Disconnect, meist mit Wartezeit (Backoff).
- **Static Files** – Unveränderliche Dateien, die direkt per HTTP ausgeliefert werden (HTML/CSS/JS).
- **WebSocket** – Dauerhafte bidirektionale Verbindung (Client ↔ Server) für Events in Echtzeit.

---

*Stand: 2026-01-08 | Version 2.5.2*
