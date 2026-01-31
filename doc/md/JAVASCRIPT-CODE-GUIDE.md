# JavaScript-Code-Guide (Dashboard-Client `app.js` + `index.html`)

Version 2.5.2 | Dashboard v2.5.1 | Stand: 2026-01-08

## 1) Architektur: Datenfluss in 4 Schritten

**Regel:** UI-Code wird wartbar, wenn der Datenfluss eindeutig ist: **Input → Parse → State → Render**.

**Beispiel im Projekt:**

1. **Input:** WebSocket empfängt `{"type":"stop"}` oder `{"type":"play","id":n}`.
2. **Parse:** `handleServerMessage()` macht `JSON.parse` + Switch auf `message.type`.
3. **State:** `state.currentId`, `state.isPlaying`, `state.audioUnlocked`, `state.preloaded`.
4. **Render:** `handleStop()` / `handlePlay(id)` aktualisieren DOM (`#current-id`, `#image-container`, Progressbar).

**Anwendung (Leitplanke):**

Jede neue Funktion (z. B. "pause", "volume", "shuffle") sollte entweder **State ändern** oder **rendern** – nicht beides quer verteilt.

## 2) Konfiguration & globaler Zustand

**Regel:** Konstanten zentralisieren, Zustand minimal halten, Zustandsänderung an wenigen Stellen.

**Beispiel:**

- `CONFIG`: `reconnectInterval = 5000`, `numMedia = 10`, `preloadConcurrency = 3`, WS-URL automatisch `ws/wss`.
- `state`: `ws`, `audioUnlocked`, `currentId`, `isPlaying`, `preloaded`, `preloadProgress`.
- Cache: `mediaCache.images[id] = HTMLImageElement`, `mediaCache.audio[id] = HTMLAudioElement`.

**Anwendung:**

Für "100 Medien": nur `CONFIG.numMedia = 100` ändern (und ggf. `preloadConcurrency` an Netzwerk/Server anpassen).

## 3) WebSocket: robust verbinden, sauber senden

**Regel:** Reconnect ist Teil des Normalbetriebs. Fehler sollen sichtbar sein, aber nicht "crashen".

**Beispiel:**

- `connectWebSocket()` setzt Handler für `onopen/onclose/onerror/onmessage`. Bei Close: Reconnect nach 5000 ms.
- `sendMessage()` sendet nur bei `readyState === OPEN`, sonst Warn-Log.

**Anwendung:**

Wenn du zusätzliche Message-Typen einführst, halte das Protokoll strikt (z. B. `type` Pflicht, Payload validieren), sonst werden UI-Bugs zu "Netzwerkproblemen".

## 4) Medien-Preloading: schnelle Wiedergabe ohne UI-Blocker

**Regel:** Preload parallel, aber begrenzt (sonst überlastest du Browser/Server).

**Beispiel (induktiv):**

- Beobachtung: Viele gleichzeitige Requests können "stottern".
- Daten: `preloadConcurrency = 3` + eigene `Semaphore`.
- Regel: `preloadAllMedia()` lädt IDs 1…`numMedia` in Batches und aktualisiert Progress.

**Detailpunkte, die gut gelöst sind:**

- Bilder: `new Image()` + `onload/onerror`, speichern in Cache.
- Audio: `new Audio()` + `oncanplaythrough`, Timeout-Fallback nach 5000 ms (speichert trotzdem ein Audio-Objekt).

**Anwendung (Tuning):**

- LAN/RPi lokal: `preloadConcurrency` eher 4–8.
- WLAN/Handy: eher 2–4 (weniger Peaks).

## 5) Playback-State-Machine: Preempt + Race-Fix

**Regel:** Bei "schnellem Umschalten" brauchst du eine eindeutige Zuordnung von Events zur aktuellen ID.

**Beispiel:**

- Preempt: `handlePlay(id)` stoppt vorheriges Audio, setzt `state.currentId = id`, startet neues.
- Race-Fix: `cachedAudio.onended = () => handleAudioEnded(id)` bindet `ended` an die ID; `handleAudioEnded(endedId)` ignoriert Events, wenn `endedId !== state.currentId`.

**Anwendung (wenn du es noch härter machen willst):**

Ergänze optional `seq` (laufende Nummer) in `play`/`ended`, dann ist auch Multi-Tab/Latency eindeutig (Server kann "alte ended" sicher ignorieren).

## 6) Audio-Unlock: iOS/Autoplay-Policies korrekt handhaben

**Regel:** Audio darf erst nach User-Geste zuverlässig starten (besonders iOS/Safari). Daher: Unlock-Button + "silent play".

**Beispiel:**

- `unlockAudio()` versucht zuerst `AudioContext` (inkl. `resume()`), dann HTML `<audio>` mit kurzem Silent-WAV (Base64) via `play() → pause()`.
- Nach Erfolg: UI umschalten (`#unlock-btn` hidden, Status-Dot), dann `preloadAllMedia()`.

**Anwendung:**

Alle Audio-Starts müssen hinter `state.audioUnlocked === true` liegen (ist im `handlePlay` so geprüft).

## 7) DOM-Integration: IDs, Rendering, Debug

**Regel:** DOM-Zugriffe einmal bündeln, Rendering über klar definierte UI-Aktionen.

**Beispiel:**

- `elements = { unlockBtn, waiting, mediaContainer, currentId, imageContainer, audio, ... }` via `getElementById`.
- Debug: `log()` schreibt in Konsole und in `#debug`, begrenzt auf 50 Zeilen.

**Anwendung (Sicherheits/Robustheits-Note):**

`innerHTML` ist ok, solange du nur kontrollierte Inhalte einsetzt. Bei künftig "freiem Text" aus dem Server: auf `textContent` wechseln (XSS-Risiko vermeiden).

## 8) Protokoll-Übersicht (Dashboard ↔ Server)

### Server → Dashboard (WebSocket)

| Message | Beschreibung |
|---------|--------------|
| `{"type":"play","id":n}` | Starte Wiedergabe für Taste n |
| `{"type":"stop"}` | Stoppe aktuelle Wiedergabe |

### Dashboard → Server (WebSocket)

| Message | Beschreibung |
|---------|--------------|
| `{"type":"ended","id":n}` | Audio für Taste n beendet |

### HTTP-Endpoints

| Endpoint | Beschreibung |
|----------|--------------|
| `GET /` | Dashboard HTML |
| `GET /media/{id}.jpg` | Bild für Taste id |
| `GET /media/{id}.mp3` | Audio für Taste id |
| `GET /status` | Server-Status (JSON) |
| `GET /health` | Health-Check (200/503) |
| `GET /test/play/{id}` | Tastendruck simulieren |

## Kurz-Checkliste für Erweiterungen

- [ ] Neues Protokollfeld: in `handleServerMessage()` validieren (Typ/Range).
- [ ] Neue UI-Anzeige: erst `elements` erweitern, dann eine dedizierte Render-Funktion.
- [ ] Preload bei 100 Medien: Concurrency und Timeout realistisch wählen (z. B. 5000–15000 ms je nach Netz).

## Glossar (alphabetisch)

- **AudioContext** – WebAudio-API-Kontext; wird oft genutzt, um Audio auf iOS nach User-Geste "freizuschalten".
- **Autoplay Policy** – Browser-Regeln, die automatisches Abspielen ohne Nutzerinteraktion blockieren (v. a. Mobilgeräte).
- **Base64** – Kodierung von Binärdaten als Text (hier: Silent-WAV als Data-URL).
- **Cache** – Zwischenspeicher, um Ressourcen (Bild/Audio) erneut ohne Netz-Latenz zu verwenden.
- **CloneNode** – DOM-Methode, um ein Element zu duplizieren (hier: Bild aus Cache als Clone einfügen).
- **Concurrency** – Anzahl gleichzeitig laufender Operationen/Requests (hier: Preload parallel, aber begrenzt).
- **DOMContentLoaded** – Event, wenn das HTML geparst ist und DOM-Elemente verfügbar sind.
- **Event Listener** – Registrierte Callback-Funktion für Events (Click, Keydown, Audio-Events, WS-Events).
- **JSON** – Textformat zur strukturierten Datenübertragung (Server↔Client Messages).
- **Preempt** – "Neues Ereignis gewinnt": laufende Wiedergabe wird sofort durch neue ID ersetzt.
- **Progress Bar** – UI-Element, das den Fortschritt zeigt (hier: Audio `currentTime/duration`).
- **Race-Condition** – Timing-Problem: ein "altes" Event (ended) trifft nach einem neuen Play ein; wird per ID-Check abgefangen.
- **Reconnect** – Automatisches Wiederverbinden nach Verbindungsabbruch (hier: nach 5000 ms).
- **Semaphore** – Synchronisationsmechanismus zur Begrenzung gleichzeitiger Tasks/Promises.
- **WebSocket (ws/wss)** – Persistente bidirektionale Verbindung; `wss` ist TLS-verschlüsselt (HTTPS).

---

*Stand: 2026-01-08 | Version 2.5.2*
