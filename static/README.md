# Web Dashboard v2.3.0

Browser-basiertes Frontend für das Auswahlpanel mit Bild- und Audio-Wiedergabe.

## Übersicht

```
┌─────────────────────────────────────────────────────────────────────┐
│                      WEB DASHBOARD                                  │
├─────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │ Header: 🎯 Auswahlpanel    [●] WebSocket  [●] Audio        │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                                                             │   │
│  │                         [003]                               │   │
│  │                                                             │   │
│  │                    ┌───────────┐                            │   │
│  │                    │           │                            │   │
│  │                    │   BILD    │                            │   │
│  │                    │           │                            │   │
│  │                    └───────────┘                            │   │
│  │                                                             │   │
│  │                    ▓▓▓▓▓▓▓▓▓░░░░░                          │   │
│  │                    1:23 / 3:45                              │   │
│  │                                                             │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │ Footer: Interaktives Auswahlpanel v2.3.0                   │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
```

## Features

- **Medien-Preloading:** Alle Bilder und Audio-Dateien werden nach Audio-Unlock vorgeladen
- **Instant Playback:** Gecachte Medien starten sofort (< 50ms)
- **iOS-kompatibel:** AudioContext + HTML5 Audio für zuverlässiges Unlock
- **Auto-Reconnect:** WebSocket verbindet automatisch neu bei Verbindungsverlust
- **Responsive Design:** Mobile-first, unterstützt Smartphones bis 4K-Displays
- **Dark/Light Theme:** Automatisch basierend auf System-Präferenz
- **Accessibility:** Reduced Motion, High Contrast, Focus-Styles

## Dateien

```
static/
├── index.html     # Hauptseite
├── app.js         # WebSocket-Client, Medien-Handling
└── styles.css     # Responsive Styles, Design-Tokens
```

## Konfiguration

In `app.js`:

```javascript
const CONFIG = {
    wsUrl: `ws://${window.location.host}/ws`,
    reconnectInterval: 5000,    // ms zwischen Reconnect-Versuchen
    debug: true,                // Debug-Panel aktivieren
    numMedia: 10,               // PROTOTYPE_MODE (100 für Produktion)
    preloadConcurrency: 3       // Parallele Preload-Requests
};
```

## Farbschema

Arduino Teal + Raspberry Pi Red (konsistent mit LaTeX-Dokumentation):

| Variable | Hex | Verwendung |
|----------|-----|------------|
| `--arduino-teal` | #00979D | Primärfarbe, Akzente |
| `--arduino-teal-dark` | #005C5F | Hover-States |
| `--raspi-red` | #C51A4A | Buttons, Highlights |
| `--raspi-green` | #75A928 | Erfolg/Verbunden |
| `--bg-primary` | #0D1117 | Hintergrund (Dark) |
| `--text-primary` | #E6EDF3 | Textfarbe |

## WebSocket-Protokoll

### Empfangen (Server → Browser)

| Nachricht | Aktion |
|-----------|--------|
| `{"type": "stop"}` | Aktuelle Wiedergabe stoppen |
| `{"type": "play", "id": 3}` | Medien-ID 3 abspielen |

### Senden (Browser → Server)

| Nachricht | Auslöser |
|-----------|----------|
| `{"type": "ended", "id": 3}` | Audio-Wiedergabe beendet |
| `{"type": "ping"}` | Heartbeat (optional) |

## Benutzerführung

### 1. Seite öffnen

```
http://rover.local:8080
```

### 2. Sound aktivieren

Browser blockieren automatische Audio-Wiedergabe. Klick auf "Sound aktivieren" entsperrt Audio und startet das Preloading.

### 3. Medien werden vorgeladen

Fortschrittsanzeige: "Lade Medien... 5/10"

### 4. Warten auf Tastendruck

Nach erfolgreichem Preload: "Warte auf Tastendruck..."

### 5. Wiedergabe

Bei Tastendruck am Panel:
- ID wird angezeigt (z.B. "003")
- Bild erscheint sofort
- Audio startet automatisch
- Fortschrittsbalken zeigt Position

## Keyboard-Shortcuts

| Taste | Funktion |
|-------|----------|
| `Space` | Play/Pause (nach Audio-Unlock) |
| `Ctrl+D` | Debug-Panel ein/ausblenden |

## Status-Indikatoren

### WebSocket-Status (links)

| Farbe | Bedeutung |
|-------|-----------|
| 🟢 Grün | Verbunden |
| 🔴 Rot | Getrennt (Reconnect läuft) |

### Audio-Status (rechts)

| Farbe | Bedeutung |
|-------|-----------|
| 🟢 Grün | Audio entsperrt, Preload OK |
| 🔴 Rot | Audio blockiert oder Fehler |

## Debug-Panel

Klick auf "Debug" (unten rechts) zeigt:
- WebSocket-Events
- Audio-Ladevorgänge
- Preload-Fortschritt
- Fehler und Warnungen

Letzte 50 Log-Einträge mit Timestamp.

## Responsive Breakpoints

| Breakpoint | Zielgerät |
|------------|-----------|
| < 480px | Smartphone Portrait |
| 480-768px | Smartphone Landscape, Tablet |
| 768-1024px | Tablet, kleiner Laptop |
| 1024-1920px | Desktop |
| 1920-2560px | Full HD, Ultra-Wide |
| > 2560px | 2K, 4K Displays |

## Browser-Kompatibilität

| Browser | Status | Hinweise |
|---------|--------|----------|
| Chrome 90+ | ✅ | Empfohlen |
| Firefox 88+ | ✅ | |
| Safari 14+ | ✅ | AudioContext-Fallback |
| Edge 90+ | ✅ | |
| iOS Safari | ✅ | Touch-Event für Unlock |
| Chrome Android | ✅ | |

## Troubleshooting

### Audio startet nicht

1. "Sound aktivieren" geklickt?
2. Browser-Konsole auf Fehler prüfen (F12)
3. Lautstärke im System/Browser prüfen

### Bild erscheint nicht

1. Medien-Dateien vorhanden? (`media/001.jpg`)
2. Server-Status prüfen: `/status`
3. Debug-Panel: Preload-Fehler?

### WebSocket verbindet nicht

1. Server läuft? (`python3 server.py`)
2. Richtige URL? (Port 8080)
3. Firewall-Regeln prüfen

### Preload dauert lange

1. Netzwerk-Verbindung prüfen
2. `preloadConcurrency` in CONFIG erhöhen (z.B. auf 5)
3. Große Mediendateien komprimieren

### iOS: Audio bricht ab

1. Bildschirm nicht sperren während Wiedergabe
2. Lautlos-Schalter deaktivieren
3. App nicht in Hintergrund wechseln

## Performance-Tipps

### Medien optimieren

```bash
# Bilder: JPEG, max 1920px Breite, 80% Qualität
convert input.png -resize 1920x -quality 80 001.jpg

# Audio: MP3, 128kbps (Stereo) oder 64kbps (Mono)
ffmpeg -i input.wav -b:a 128k 001.mp3
```

### Preload-Strategie

- **Prototype (10 Medien):** Alle vorladen, `preloadConcurrency: 3`
- **Production (100 Medien):** Alle vorladen, `preloadConcurrency: 5`
- **Langsames Netz:** `preloadConcurrency: 1-2`, Timeout anpassen

## Changelog

### v2.3.0 (2025-01-01)

- NEU: Medien-Preloading nach Audio-Unlock
- NEU: Preload-Statusanzeige
- NEU: Gecachte Audio/Image-Objekte für sofortige Wiedergabe
- Optimiert: handlePlay() nutzt vorgeladene Medien

### v2.2.5 (2025-12-31)

- Farbschema: Arduino Teal + Raspberry Pi Red
- Konsistenz mit LaTeX-Dokumentation (farbschema.tex)
- Responsive Breakpoints für 4K

### v2.2.0 (2025-12-30)

- Dark/Light Theme Support
- High Contrast Mode
- Safe Area für iPhone Notch

## Lizenz

MIT License

## Autor

Jan Unger - Selection Panel Projekt
