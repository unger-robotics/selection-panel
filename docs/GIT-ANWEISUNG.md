# Git-Anweisung: Selection-Panel

> **Repository:** `https://github.com/unger-robotics/selection-panel`  
> **Architektur:** ESP32-S3 (Firmware) + Raspberry Pi 5 (Server/Dashboard)

---

## 1. Erstmaliges Setup (neues Projekt hochladen)

```bash
# Im Projektverzeichnis
cd ./selection-panel

# Git initialisieren
git init

# Alle Dateien stagen
git add .

# Erster Commit
git commit -m "feat: Initial commit – ESP32-S3 Firmware, Pi5 Server, Dashboard"

# Branch umbenennen (falls nötig)
git branch -M main

# Remote hinzufügen
git remote add origin git@github.com:unger-robotics/selection-panel.git

# Pushen
git push -u origin main
```

---

## 2. Repository klonen (bestehendes Projekt)

```bash
# HTTPS (Standard)
git clone https://github.com/unger-robotics/selection-panel.git
cd selection-panel

# SSH (mit hinterlegtem Key)
git clone git@github.com:unger-robotics/selection-panel.git
cd selection-panel
```

---

## 3. Täglicher Workflow

### Aktuellen Stand holen

```bash
# Vor jeder Arbeit: Remote-Änderungen integrieren
git pull origin main
```

### Änderungen committen

```bash
# Status prüfen – welche Dateien wurden geändert?
git status

# Einzelne Datei stagen
git add button_panel_firmware/src/main.cpp

# Alle Änderungen stagen
git add .

# Commit mit aussagekräftiger Nachricht
git commit -m "feat(firmware): Debounce-Logik für 100 Taster implementiert"

# Zum Remote pushen
git push origin main
```

---

## 4. Commit-Konventionen

Wir nutzen **Conventional Commits** für lesbare Historie:

| Präfix     | Verwendung                          | Beispiel                                      |
|------------|-------------------------------------|-----------------------------------------------|
| `feat`     | Neue Funktion                       | `feat(server): WebSocket-Broadcast hinzugefügt` |
| `fix`      | Bugfix                              | `fix(firmware): LED-Index off-by-one korrigiert` |
| `docs`     | Dokumentation                       | `docs: HARDWARE.md um Pinout erweitert`       |
| `refactor` | Code-Umstrukturierung               | `refactor(firmware): shift_register modularisiert` |
| `test`     | Tests hinzufügen/ändern             | `test(server): Unit-Tests für /api/select`    |
| `chore`    | Build, Dependencies, Tooling        | `chore: platformio.ini auf ESP-IDF 5.1 aktualisiert` |

**Scope** (optional): `firmware`, `server`, `dashboard`, `docs`, `media`

---

## 5. Branching-Strategie

```
main ────────────────────────────────────────────►
        \                            /
         feature/led-animation ─────►
        \                                /
         fix/uart-buffer-overflow ──────►
```

### Feature-Branch erstellen

```bash
# Neuen Branch anlegen und wechseln
git checkout -b feature/websocket-reconnect

# Arbeit erledigen, dann committen
git add .
git commit -m "feat(dashboard): Auto-Reconnect bei WebSocket-Verbindungsverlust"

# Branch pushen
git push -u origin feature/websocket-reconnect

# Nach Review: Zurück zu main, Branch mergen
git checkout main
git pull origin main
git merge feature/websocket-reconnect
git push origin main

# Branch lokal und remote löschen
git branch -d feature/websocket-reconnect
git push origin --delete feature/websocket-reconnect
```

---

## 6. Häufige Situationen

### Letzte Commit-Nachricht korrigieren

```bash
git commit --amend -m "fix(firmware): Korrekter Text hier"
```

### Änderungen temporär parken (Stash)

```bash
# Aktuelle Änderungen zwischenspeichern
git stash push -m "WIP: LED-Mapping unfertig"

# Stash-Liste anzeigen
git stash list

# Änderungen wiederherstellen
git stash pop
```

### Datei aus Staging entfernen

```bash
git restore --staged button_panel_firmware/src/main.cpp
```

### Lokale Änderungen verwerfen

```bash
# Einzelne Datei zurücksetzen
git restore button_panel_firmware/src/main.cpp

# ACHTUNG: Alle ungestagten Änderungen verwerfen
git restore .
```

---

## 7. Projektspezifische .gitignore

Diese Dateien/Ordner gehören **nicht** ins Repository:

```gitignore
# PlatformIO
button_panel_firmware/.pio/
button_panel_firmware/.vscode/

# Python
__pycache__/
*.pyc
venv/

# System
.DS_Store
*.log

# IDE
.idea/
*.code-workspace

# Generierte Dokumentation (optional)
docs/_site/
```

---

## 8. Nützliche Aliase

Diese Shortcuts beschleunigen die tägliche Arbeit:

```bash
# In ~/.gitconfig oder ~/.zshrc einfügen
git config --global alias.st "status -sb"
git config --global alias.co "checkout"
git config --global alias.br "branch"
git config --global alias.ci "commit"
git config --global alias.lg "log --oneline --graph --decorate -15"
```

**Verwendung:**

```bash
git st      # Kompakter Status
git lg      # Übersichtliche Historie
```

---

## 9. Quick-Reference

| Aktion                        | Befehl                                    |
|-------------------------------|-------------------------------------------|
| Status anzeigen               | `git status`                              |
| Historie anzeigen             | `git log --oneline -10`                   |
| Diff anzeigen                 | `git diff`                                |
| Remote-Branches anzeigen      | `git branch -r`                           |
| Tag erstellen                 | `git tag -a v1.0.0 -m "Release 1.0.0"`    |
| Tag pushen                    | `git push origin v1.0.0`                  |
| Alle Tags pushen              | `git push origin --tags`                  |

---

## 10. Deployment auf Raspberry Pi

```bash
# Auf dem Pi: Repository klonen oder aktualisieren
ssh pi@raspberrypi
cd ~/selection-panel
git pull origin main

# Server neu starten
sudo systemctl restart selection-panel.service
```
