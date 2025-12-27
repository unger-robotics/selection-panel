# DASHBOARD — Web-Frontend

> Vollbild-Anzeige für Bild- und Audio-Wiedergabe.

| Metadaten | Wert |
|-----------|------|
| Version | 2.2.5 |
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

---

## 2 Dateistruktur

```
static/
├── index.html      # Hauptseite
├── styles.css      # Styling mit Design Tokens
└── app.js          # WebSocket-Client
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
| `{"type":"stop"}` | Audio stoppen, Bild ausblenden |
| `{"type":"play","id":42}` | Bild 042.jpg laden, Audio 042.mp3 abspielen |

### 4.2 Senden (Browser → Server)

| Nachricht | Auslöser |
|-----------|----------|
| `{"type":"ended","id":42}` | Audio fertig abgespielt |

### 4.3 Auto-Reconnect

```javascript
ws.onclose = () => {
    setTimeout(connectWebSocket, 5000);  // Nach 5 s neu verbinden
};
```

---

## 5 Audio-Unlock

Browser blockieren automatische Audio-Wiedergabe – die **Autoplay Policy**. Erst nach einer Nutzer-Interaktion darf Audio starten.

**Lösung:** Der Unlock-Button spielt ein stilles Audio ab. Danach ist Audio entsperrt.

```javascript
function unlockAudio() {
    audio.src = 'data:audio/wav;base64,...';  // Stilles WAV
    audio.play().then(() => {
        state.audioUnlocked = true;
        unlockButton.style.display = 'none';
    });
}
```

---

## 6 Keyboard-Shortcuts

| Taste | Funktion |
|-------|----------|
| `Space` | Play/Pause |
| `Ctrl+D` | Debug-Panel ein/aus |

---

## 7 Status-Indikatoren

Zwei Punkte oben rechts zeigen den Verbindungsstatus:

| Indikator | Grün | Rot |
|-----------|------|-----|
| WebSocket | Verbunden | Getrennt |
| Audio | Entsperrt | Gesperrt |

---

## 8 Debug-Panel

Das Debug-Panel (Ctrl+D) zeigt alle Events:

```
[14:32:05] WS: {"type":"play","id":5}
[14:32:05] Audio gestartet: 005.mp3
[14:32:12] Audio beendet
[14:32:12] TX: {"type":"ended","id":5}
```

---

## 9 Responsive Design

| Breakpoint | Ziel |
|------------|------|
| < 768px | Mobile |
| 768–1199px | Tablet |
| 1200–1919px | Desktop |
| ≥ 1920px | Full HD / 4K |

---

## 10 Accessibility

| Feature | CSS-Query |
|---------|-----------|
| Reduzierte Animationen | `prefers-reduced-motion` |
| Hoher Kontrast | `prefers-contrast: high` |
| Light Mode | `prefers-color-scheme: light` |
| iPhone Notch | `env(safe-area-inset-*)` |

---

## 11 Features

- [x] WebSocket mit Auto-Reconnect
- [x] Audio-Unlock für Autoplay-Policy
- [x] Fortschrittsanzeige mit Zeitanzeige
- [x] Status-Indikatoren (WebSocket, Audio)
- [x] Debug-Panel mit Log-History
- [x] Responsive Design (Mobile → 4K)
- [x] Dark/Light Theme Support
- [x] Keyboard-Shortcuts
