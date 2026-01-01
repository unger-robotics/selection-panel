# DASHBOARD — Web-Frontend

> Vollbild-Anzeige für Bild- und Audio-Wiedergabe mit Medien-Preloading.

| Metadaten | Wert |
|-----------|------|
| Version | 2.3.0 |
| Stand | 2025-01-01 |
| Status | ✅ Prototyp funktionsfähig |
| Framework | Vanilla JavaScript |
| Farbschema | Arduino Teal + Raspberry Pi Red |

---

## 1 Überblick

Das Dashboard ist eine **Single-Page-Application (SPA)** – eine Webanwendung, die nur einmal geladen wird. Alle Updates kommen per WebSocket, ohne Seitenneuladen.

```
┌─────────────────────────────────────────────────────────────┐
│                      Browser Dashboard                       │
├─────────────────────────────────────────────────────────────┤
│   ┌─────────────┐    WebSocket     ┌─────────────────────┐  │
│   │   app.js    │ ◄──────────────► │  Server :8080       │  │
│   │  • RX: stop │                  │                     │  │
│   │  • RX: play │                  │                     │  │
│   │  • TX: ended│                  └─────────────────────┘  │
│   └──────┬──────┘                                           │
│          ▼                                                  │
│   ┌─────────────────────────────────────────────────────┐   │
│   │  Header │ Status │ Bild │ Audio │ Progress │ Debug  │   │
│   └─────────────────────────────────────────────────────┘   │
│          ▲                                                  │
│   ┌──────┴──────┐                                           │
│   │ mediaCache  │  ← NEU: Vorgeladene Medien               │
│   │ • images[]  │                                           │
│   │ • audio[]   │                                           │
│   └─────────────┘                                           │
└─────────────────────────────────────────────────────────────┘
```

**URL:** `http://rover:8080/`

---

## 2 Dateistruktur

```
static/
├── index.html      # Hauptseite (v2.2.5)
├── styles.css      # Styling mit Design Tokens (v2.2.5)
└── app.js          # WebSocket-Client mit Preloading (v2.3.0)
```

**Design Tokens:** Zentral definierte Farbwerte als CSS-Variablen. Ändert man `--arduino-teal`, ändert sich die Farbe überall.

---

## 3 Neu in v2.3.0: Medien-Preloading

Nach dem Audio-Unlock werden alle Medien vorgeladen. Das ermöglicht sofortige Wiedergabe ohne Ladezeit.

### 3.1 Ablauf

```
[Sound aktivieren] → Audio-Unlock → Preload startet
                                         ↓
                              "Lade Medien... 3/10"
                                         ↓
                              "Warte auf Tastendruck..."
                                         ↓
                     [Tastendruck] → Sofortige Wiedergabe (< 50ms)
```

### 3.2 Cache-Struktur

```javascript
const mediaCache = {
    images: {},   // id -> HTMLImageElement (vorgeladen)
    audio: {}     // id -> HTMLAudioElement (vorgeladen)
};
```

### 3.3 Konfiguration

```javascript
const CONFIG = {
    wsUrl: `ws://${window.location.host}/ws`,
    reconnectInterval: 5000,
    debug: true,
    numMedia: 10,            // PROTOTYPE_MODE (100 für Produktion)
    preloadConcurrency: 3    // Parallele Preload-Requests
};
```

**preloadConcurrency:** Begrenzt gleichzeitige Downloads. Zu hoch = Browser-Überlastung, zu niedrig = langsames Preloading.

### 3.4 Semaphore für Parallelität

```javascript
class Semaphore {
    constructor(max) { this.max = max; this.current = 0; this.queue = []; }
    async acquire() { /* wartet bis Slot frei */ }
    release() { /* gibt Slot frei */ }
}
```

---

## 4 Farbschema

| Farbe | Hex | Verwendung |
|-------|-----|------------|
| Arduino Teal | `#00979D` | Titel, Links, Akzente |
| Arduino Teal Dark | `#005C5F` | Hover-States |
| Raspberry Red | `#C51A4A` | Unlock-Button, Warnungen |
| Success Green | `#75A928` | Status: Verbunden |
| GitHub Dark | `#0D1117` | Hintergrund |

---

## 5 WebSocket-Protokoll

### 5.1 Empfangen (Server → Browser)

| Nachricht | Aktion |
|-----------|--------|
| `{"type":"stop"}` | Audio stoppen, Reset |
| `{"type":"play","id":5}` | Bild 005.jpg laden, Audio 005.mp3 abspielen |

**Nummerierung:** Die `id` ist 1-basiert (1-100). Das Dashboard formatiert sie als 3-stellige Zahl für Dateinamen.

### 5.2 Senden (Browser → Server)

| Nachricht | Auslöser |
|-----------|----------|
| `{"type":"ended","id":5}` | Audio fertig abgespielt |
| `{"type":"ping"}` | Heartbeat (optional) |

### 5.3 Auto-Reconnect

```javascript
ws.onclose = () => {
    setTimeout(connectWebSocket, 5000);  // Nach 5 s neu verbinden
};
```

---

## 6 Medien-Laden (optimiert)

### 6.1 Mit Cache (v2.3.0)

```javascript
function handlePlay(id) {
    const id3 = id.toString().padStart(3, '0');
    
    // Bild aus Cache (instant!)
    const cachedImg = mediaCache.images[id];
    if (cachedImg) {
        elements.imageContainer.innerHTML = '';
        elements.imageContainer.appendChild(cachedImg.cloneNode());
    }
    
    // Audio aus Cache (instant!)
    const cachedAudio = mediaCache.audio[id];
    if (cachedAudio) {
        cachedAudio.currentTime = 0;
        cachedAudio.play();
    }
}
```

### 6.2 Fallback (ohne Cache)

Falls Medien nicht im Cache, werden sie direkt geladen:

```javascript
// Fallback: Neu laden
elements.imageContainer.innerHTML = `<img src="/media/${id3}.jpg">`;
elements.audio.src = `/media/${id3}.mp3`;
elements.audio.play();
```

**Dateipfade:**
- Bild: `/media/001.jpg` bis `/media/010.jpg` (Prototyp)
- Audio: `/media/001.mp3` bis `/media/010.mp3` (Prototyp)

---

## 7 Audio-Unlock

Browser blockieren automatische Audio-Wiedergabe – die **Autoplay Policy**. Erst nach einer Nutzer-Interaktion darf Audio starten.

**Lösung:** Der Unlock-Button spielt ein stilles Audio ab. Danach ist Audio entsperrt und das Preloading startet.

```javascript
async function unlockAudio() {
    // 1. AudioContext für iOS
    const ctx = new AudioContext();
    await ctx.resume();
    
    // 2. HTML Audio Unlock
    audio.src = 'data:audio/wav;base64,...';  // Stilles WAV
    await audio.play();
    
    state.audioUnlocked = true;
    
    // 3. NEU: Preloading starten
    await preloadAllMedia();
}
```

**Wichtig:** Vor dem ersten Tastendruck muss der "Sound aktivieren" Button geklickt werden!

---

## 8 Keyboard-Shortcuts

| Taste | Funktion |
|-------|----------|
| `Space` | Play/Pause (nutzt gecachtes Audio) |
| `Ctrl+D` | Debug-Panel ein/aus |

---

## 9 Status-Indikatoren

Zwei Punkte oben rechts zeigen den Verbindungsstatus:

| Indikator | Grün | Rot |
|-----------|------|-----|
| WebSocket | Verbunden | Getrennt |
| Audio | Entsperrt + Preload OK | Gesperrt/Fehler |

---

## 10 Debug-Panel

Das Debug-Panel (Button unten rechts oder Ctrl+D) zeigt alle Events:

```
[22:33:20] Preloading 10 Medien...
[22:33:22] Preload abgeschlossen: 10/10 OK, 0 Fehler (1823ms)
[22:33:26] RX: {"type": "play", "id": 5}
[22:33:26] Bild aus Cache: 005 (instant)
[22:33:26] Audio aus Cache gestartet: 005 (12ms)
[22:33:27] Audio beendet: 5
[22:33:27] TX: {"type":"ended","id":5}
```

**Events:**
- `RX:` – Vom Server empfangen
- `TX:` – An Server gesendet
- `Preloading` – Medien werden geladen
- `aus Cache` – Gecachte Medien verwendet
- `neu geladen` – Fallback ohne Cache

---

## 11 Anzeige-Elemente

| Element | Beschreibung |
|---------|--------------|
| Header | "🎯 Auswahlpanel" + Status-Dots |
| Unlock-Button | "🔈 Sound aktivieren" |
| Waiting-Text | "Lade Medien... 5/10" oder "Warte auf Tastendruck..." |
| Current ID | 3-stellige Nummer (001-100) |
| Bild | Vollbild-Anzeige des aktuellen Bildes |
| Progress Bar | Audio-Fortschritt |
| Zeit-Anzeige | "0:00 / 1:05" Format |
| Debug-Panel | Event-Log (toggle mit Button) |

---

## 12 Responsive Design

| Breakpoint | Ziel |
|------------|------|
| < 480px | Smartphone Portrait |
| 480–768px | Smartphone Landscape |
| 768–1024px | Tablet |
| 1024–1920px | Desktop |
| 1920–2560px | Full HD |
| ≥ 2560px | 2K / 4K |

---

## 13 Accessibility

| Feature | CSS-Query |
|---------|-----------|
| Reduzierte Animationen | `prefers-reduced-motion` |
| Hoher Kontrast | `prefers-contrast: high` |
| Light Mode | `prefers-color-scheme: light` |
| iPhone Notch | `env(safe-area-inset-*)` |
| Focus-Styles | `:focus-visible` |

---

## 14 Features

- [x] WebSocket mit Auto-Reconnect (5s Intervall)
- [x] Audio-Unlock für Autoplay-Policy (AudioContext + HTML5)
- [x] **NEU:** Medien-Preloading nach Unlock
- [x] **NEU:** Cache für sofortige Wiedergabe (< 50ms)
- [x] **NEU:** Preload-Fortschrittsanzeige
- [x] Fortschrittsanzeige mit Zeitanzeige
- [x] Status-Indikatoren (WebSocket, Audio)
- [x] Debug-Panel mit Log-History (max. 50 Einträge)
- [x] Responsive Design (Mobile → 4K)
- [x] Dark/Light Theme Support
- [x] Keyboard-Shortcuts (Space, Ctrl+D)
- [x] 1-basierte ID-Anzeige (001-100)

---

## 15 Bekannte Einschränkungen

| Problem | Workaround |
|---------|------------|
| Audio startet nicht | "Sound aktivieren" Button klicken |
| Preload dauert lange | `preloadConcurrency` erhöhen oder Medien komprimieren |
| WebSocket getrennt | Auto-Reconnect nach 5s |
| Bild nicht gefunden | Placeholder "Bild nicht gefunden" |
| iOS Audio bricht ab | Bildschirm nicht sperren, Lautlos-Schalter aus |

---

## 16 Browser-Kompatibilität

| Browser | Status | Hinweise |
|---------|--------|----------|
| Chrome/Chromium 90+ | ✅ Getestet | Empfohlen |
| Firefox 88+ | ✅ Unterstützt | |
| Safari 14+ | ✅ Unterstützt | AudioContext-Fallback |
| Edge 90+ | ✅ Unterstützt | |
| Mobile Safari (iOS) | ✅ Unterstützt | Touch-Event für Unlock |
| Mobile Chrome | ✅ Unterstützt | |

---

## 17 Changelog

### v2.3.0 (2025-01-01)

- NEU: Medien-Preloading nach Audio-Unlock
- NEU: Preload-Statusanzeige ("Lade Medien... 5/10")
- NEU: Gecachte Audio/Image-Objekte für sofortige Wiedergabe
- NEU: Semaphore für begrenzte Parallelität
- Optimiert: handlePlay() nutzt vorgeladene Medien

### v2.2.5 (2025-12-31)

- Farbschema: Arduino Teal + Raspberry Pi Red
- Konsistenz mit LaTeX-Dokumentation (farbschema.tex)

### v2.2.4 (2025-12-30)

- 1-basierte ID-Anzeige (001-100)
- Responsive Breakpoints für 4K
