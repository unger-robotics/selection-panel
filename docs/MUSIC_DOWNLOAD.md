# Selection Panel - Musik Download (News/Background)

## 10 distinkte Musik-Tracks von Pixabay

Alle Tracks sind **royalty-free** und brauchen **keine Attribution**.

### Empfohlene Tracks (30s - 2min, professionell, distinkt)

| # | Track | Künstler | Länge | Link |
|---|-------|----------|-------|------|
| 1 | News Background Music | Music_Unlimited | 1:00 | https://pixabay.com/music/introoutro-news-background-music-144801/ |
| 2 | Breaking News Intro Logo | Ivan_Luzan | 0:14 | https://pixabay.com/music/introoutro-breaking-news-intro-logo-254490/ |
| 3 | Breaking News - Background Music | SigmaMusicArt | 1:22 | https://pixabay.com/music/main-title-breaking-news-background-music-252187/ |
| 4 | Breaking News Symphony | Oqu | 1:36 | https://pixabay.com/music/main-title-breaking-news-symphony-252428/ |
| 5 | News Background Music | SigmaMusicArt | 1:20 | https://pixabay.com/music/upbeat-news-background-music-252280/ |
| 6 | Breaking News Intro | Humanoide_Media | 0:30 | https://pixabay.com/music/main-title-breaking-news-intro-237493/ |
| 7 | Classic Breaking News | FabienRoch | 0:29 | https://pixabay.com/music/main-title-classic-breaking-news-197869/ |
| 8 | Flash News - Opening Theme | Sonican | 1:42 | https://pixabay.com/music/corporate-flash-news-opening-theme-252047/ |
| 9 | Good News | SoulProdMusic | 1:54 | https://pixabay.com/music/corporate-good-news-148692/ |
| 10 | News Information | Sonican | 1:54 | https://pixabay.com/music/corporate-news-information-296574/ |

---

## Download-Anleitung

### Schritt 1: Manuell herunterladen

1. Jeden Link öffnen
2. Grünen **"Download"** Button klicken
3. **"MP3"** wählen
4. Speichern als `download1.mp3` bis `download10.mp3`

### Schritt 2: Umbenennen

```bash
cd ~/Downloads  # oder wo die Dateien liegen

for i in {1..10}; do
    src="download${i}.mp3"
    dst=$(printf "%03d.mp3" "$i")
    [ -f "$src" ] && mv "$src" "$dst" && echo "✓ $src → $dst"
done
```

### Schritt 3: Auf Raspberry Pi kopieren

```bash
scp 0*.mp3 pi@rover.local:~/selection-panel/media/
```

---

## Alternative: Verschiedene Genres

Falls du mehr Abwechslung willst, hier gemischte Genres:

| # | Track | Genre | Länge | Link |
|---|-------|-------|-------|------|
| 1 | News Background | Corporate | 1:00 | https://pixabay.com/music/introoutro-news-background-music-144801/ |
| 2 | Epic Cinematic | Orchestral | 1:30 | https://pixabay.com/music/main-title-epic-cinematic-trailer-161621/ |
| 3 | Lofi Study | Chill | 2:00 | https://pixabay.com/music/beats-lofi-study-112191/ |
| 4 | Upbeat Corporate | Business | 1:45 | https://pixabay.com/music/corporate-upbeat-corporate-background-136269/ |
| 5 | Acoustic Guitar | Folk | 2:30 | https://pixabay.com/music/acoustic-group-acoustic-guitar-soundtrack-278377/ |
| 6 | Electronic Future | Synth | 1:20 | https://pixabay.com/music/future-bass-electronic-future-beats-117997/ |
| 7 | Piano Emotional | Classical | 2:00 | https://pixabay.com/music/solo-piano-sad-piano-background-260345/ |
| 8 | Jazz Cafe | Jazz | 2:00 | https://pixabay.com/music/traditional-jazz-the-best-jazz-club-in-new-orleans-164472/ |
| 9 | Funk Groove | Funk | 1:30 | https://pixabay.com/music/funk-funk-groovy-loop-120bpm-9366/ |
| 10 | Retro Game | 8-bit | 0:45 | https://pixabay.com/music/video-games-retro-game-arcade-short-236130/ |

---

## Tipp: Tracks testen

```bash
# Auf dem Raspberry Pi:
cd ~/selection-panel/media

# Alle Tracks abspielen
for f in 0*.mp3; do
    echo "▶ Playing: $f"
    mpv --no-video --length=5 "$f" 2>/dev/null  # Nur erste 5 Sekunden
done

# Einzelnen Track testen
mpv --no-video 001.mp3
```
