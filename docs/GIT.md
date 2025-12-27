# Git-Workflow

> Repository: `github.com:unger-robotics/selection-panel`

---

## Erstmaliges Setup

```bash
cd ~/selection-panel
git init
git add .
git commit -m "feat: Initial commit"
git branch -M main
git remote add origin git@github.com:unger-robotics/selection-panel.git
git push -u origin main
```

## Klonen (bestehendes Repo)

```bash
git clone git@github.com:unger-robotics/selection-panel.git
```

---

## Täglicher Workflow

```bash
git pull                    # Änderungen holen
git add .                   # Änderungen stagen
git commit -m "feat: ..."   # Committen
git push                    # Pushen
```

---

## Commit-Konventionen

| Präfix | Verwendung | Beispiel |
|--------|------------|----------|
| `feat` | Neue Funktion | `feat(server): WebSocket-Broadcast` |
| `fix` | Bugfix | `fix(firmware): LED-Index korrigiert` |
| `docs` | Dokumentation | `docs: HARDWARE.md erweitert` |
| `refactor` | Umstrukturierung | `refactor: shift_register modularisiert` |
| `chore` | Build, Tooling | `chore: platformio.ini aktualisiert` |

**Scopes:** `firmware`, `server`, `dashboard`, `docs`

---

## Branching

```bash
# Feature-Branch
git checkout -b feature/led-animation
git add . && git commit -m "feat: LED-Animation"
git push -u origin feature/led-animation

# Nach Review: Mergen
git checkout main
git pull
git merge feature/led-animation
git push
git branch -d feature/led-animation
```

---

## Häufige Aktionen

```bash
git status                  # Was hat sich geändert?
git log --oneline -10       # Letzte 10 Commits
git diff                    # Änderungen anzeigen
git stash                   # Änderungen parken
git stash pop               # Änderungen zurückholen
git commit --amend -m "..." # Letzten Commit korrigieren
```

---

## Deployment (Mac → Pi)

```bash
# Dateien synchronisieren
rsync -avz --delete \
  --exclude='venv' \
  --exclude='.git' \
  --exclude='__pycache__' \
  . pi@rover:~/selection-panel/

# Server neu starten
ssh rover 'sudo systemctl restart selection-panel'
```

---

## .gitignore

```gitignore
button_panel_firmware/.pio/
button_panel_firmware/.vscode/
__pycache__/
*.pyc
venv/
.DS_Store
```
