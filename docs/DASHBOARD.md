# DASHBOARD — Web-Frontend

> Vollbild-Anzeige für Bild- und Audio-Wiedergabe.

| Metadaten | Wert |
|-----------|------|
| Version | 2.2.4 |
| Stand | 2025-12-30 |
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
└─────────────────────────────────────────────────────────────┘
```

**URL:** `http://rover:8080/`

---

## 2 Dateistruktur

```
static/
├── index.html      # Hauptseite
├── styles.css      # Styling mit Design Tokens
└── app.js          # WebSocket-Client (v2.2.4)
```

**Design Tokens:** Zentral definierte Farbwerte als CSS-Variablen. Ändert man `--arduino-teal`, ändert sich die Farbe überall.

---

## 3 Farbschema

| Farbe | Hex | Verwendung |
|-------|-----|------------|
| Arduino Teal | `#00979D` | Titel, Links, Akzente |
| Raspberry Red | `#C51A4A` | Unlock-Button, Warnungen |
| Success Green | `#75A928` | Status: Verbunden |
| GitHub Dark | `#0D1117` | Hintergrund |

---

## 4 WebSocket-Protokoll

### 4.1 Empfangen (Server → Browser)

| Nachricht | Aktion |
|-----------|--------|
| `{"type":"stop"}` | Audio stoppen, Reset |
| `{"type":"play","id":5}` | Bild 005.jpg laden, Audio 005.mp3 abspielen |

**Nummerierung:** Die `id` ist 1-basiert (1-100). Das Dashboard formatiert sie als 3-stellige Zahl für Dateinamen.

### 4.2 Senden (Browser → Server)

| Nachricht | Auslöser |
|-----------|----------|
| `{"type":"ended","id":5}` | Audio fertig abgespielt |

### 4.3 Auto-Reconnect

```javascript
ws.onclose = () => {
    setTimeout(connectWebSocket, 5000);  // Nach 5 s neu verbinden
};
```

---

## 5 Medien-Laden (1-basiert!)

```javascript
function handlePlay(id) {
    // id ist 1-basiert (1, 2, 3, ...)
    const id3 = id.toString().padStart(3, '0');  // "001", "002", ...
    
    // Bild laden
    elements.imageContainer.innerHTML = 
        `<img src="/media/${id3}.jpg" alt="${id3}">`;
    
    // Audio abspielen
    elements.audio.src = `/media/${id3}.mp3`;
    elements.audio.play();
}
```

**Dateipfade:**
- Bild: `/media/001.jpg` bis `/media/010.jpg` (Prototyp)
- Audio: `/media/001.mp3` bis `/media/010.mp3` (Prototyp)

---

## 6 Audio-Unlock

Browser blockieren automatische Audio-Wiedergabe – die **Autoplay Policy**. Erst nach einer Nutzer-Interaktion darf Audio starten.

**Lösung:** Der Unlock-Button spielt ein stilles Audio ab. Danach ist Audio entsperrt.

```javascript
function unlockAudio() {
    audio.src = 'data:audio/wav;base64,...';  // Stilles WAV
    audio.play().then(() => {
        state.audioUnlocked = true;
        unlockButton.classList.add('hidden');
        waiting.classList.remove('hidden');
    });
}
```

**Wichtig:** Vor dem ersten Tastendruck muss der "Sound aktivieren" Button geklickt werden!

---

## 7 Keyboard-Shortcuts

| Taste | Funktion |
|-------|----------|
| `Space` | Play/Pause |
| `Ctrl+D` | Debug-Panel ein/aus |

---

## 8 Status-Indikatoren

Zwei Punkte oben rechts zeigen den Verbindungsstatus:

| Indikator | Grün | Rot |
|-----------|------|-----|
| WebSocket | Verbunden | Getrennt |
| Audio | Entsperrt | Gesperrt |

---

## 9 Debug-Panel

Das Debug-Panel (Button unten rechts oder Ctrl+D) zeigt alle Events:

```
[22:33:26] RX: {"type": "play", "id": 5}
[22:33:26] PLAY: 5
[22:33:26] Audio gestartet: 005
[22:33:27] Audio beendet: 5
[22:33:27] TX: {"type":"ended","id":5}
```

**Events:**
- `RX:` – Vom Server empfangen
- `TX:` – An Server gesendet
- `PLAY:` – Wiedergabe gestartet
- `STOP` – Wiedergabe gestoppt

---

## 10 Anzeige-Elemente

| Element | Beschreibung |
|---------|--------------|
| Header | "🎯 Auswahlpanel" + Status-Dots |
| Current ID | 3-stellige Nummer (001-010) |
| Bild | Vollbild-Anzeige des aktuellen Bildes |
| Progress Bar | Audio-Fortschritt |
| Zeit-Anzeige | "0:00 / 1:05" Format |
| Debug-Panel | Event-Log (toggle mit Button) |

---

## 11 Responsive Design

| Breakpoint | Ziel |
|------------|------|
| < 768px | Mobile |
| 768–1199px | Tablet |
| 1200–1919px | Desktop |
| ≥ 1920px | Full HD / 4K |

---

## 12 Accessibility

| Feature | CSS-Query |
|---------|-----------|
| Reduzierte Animationen | `prefers-reduced-motion` |
| Hoher Kontrast | `prefers-contrast: high` |
| Light Mode | `prefers-color-scheme: light` |
| iPhone Notch | `env(safe-area-inset-*)` |

---

## 13 Features

- [x] WebSocket mit Auto-Reconnect (5s Intervall)
- [x] Audio-Unlock für Autoplay-Policy
- [x] Fortschrittsanzeige mit Zeitanzeige
- [x] Status-Indikatoren (WebSocket, Audio)
- [x] Debug-Panel mit Log-History (max. 50 Einträge)
- [x] Responsive Design (Mobile → 4K)
- [x] Dark/Light Theme Support
- [x] Keyboard-Shortcuts (Space, Ctrl+D)
- [x] 1-basierte ID-Anzeige (001-100)

---

## 14 Bekannte Einschränkungen

| Problem | Workaround |
|---------|------------|
| Audio startet nicht | "Sound aktivieren" Button klicken |
| WebSocket getrennt | Auto-Reconnect nach 5s |
| Bild nicht gefunden | Placeholder "Bild nicht gefunden" |

---

## 15 Browser-Kompatibilität

| Browser | Status |
|---------|--------|
| Chrome/Chromium | ✅ Getestet |
| Firefox | ✅ Unterstützt |
| Safari | ✅ Unterstützt |
| Edge | ✅ Unterstützt |
| Mobile Safari | ✅ Unterstützt |
| Mobile Chrome | ✅ Unterstützt |
