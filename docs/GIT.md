# Git-Workflow

Repository: `github.com:unger-robotics/selection-panel`

---

## Repository klonen

```bash
git clone git@github.com:unger-robotics/selection-panel.git
cd selection-panel
```

> **Voraussetzung:** SSH-Key für GitHub eingerichtet → [SSH.md](SSH.md#4-github-optional)

---

## Erstmaliges Setup (neues Repo)

```bash
cd ~/selection-panel
git init
git add .
git commit -m "feat: Initial commit"
git branch -M main
git remote add origin git@github.com:unger-robotics/selection-panel.git
git push -u origin main
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
|:-------|:-----------|:---------|
| `feat` | Neue Funktion | `feat(server): WebSocket-Broadcast` |
| `fix` | Bugfix | `fix(firmware): LED-Index korrigiert` |
| `docs` | Dokumentation | `docs: HARDWARE.md erweitert` |
| `perf` | Performance | `perf: Latenz-Optimierung` |
| `refactor` | Umstrukturierung | `refactor: shift_register modularisiert` |
| `chore` | Build, Tooling | `chore: requirements.txt vereinfacht` |

**Scopes:** `firmware`, `server`, `dashboard`, `docs`

---

## Branching

```bash
# Feature-Branch erstellen
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

## Häufige Befehle

| Befehl | Aktion |
|:-------|:-------|
| `git status` | Was hat sich geändert? |
| `git log --oneline -10` | Letzte 10 Commits |
| `git diff` | Änderungen anzeigen |
| `git stash` | Änderungen parken |
| `git stash pop` | Änderungen zurückholen |
| `git commit --amend -m "..."` | Letzten Commit korrigieren |
| `git tag -a v2.4.2 -m "..."` | Release-Tag erstellen |
| `git push origin v2.4.2` | Tag pushen |

---

## Deployment (Mac → Pi)

```bash
# Option 1: rsync
rsync -avz --delete \
  --exclude='venv' \
  --exclude='.git' \
  --exclude='__pycache__' \
  . pi@rover:~/selection-panel/

ssh rover 'sudo systemctl restart selection-panel'

# Option 2: Git (empfohlen)
ssh rover 'cd ~/selection-panel && git pull && sudo systemctl restart selection-panel'
```

---

## .gitignore

```gitignore
button_panel_firmware/.pio/
button_panel_firmware/.vscode/
hardwaretest_firmware/*/.pio/
hardwaretest_firmware/*/.vscode/
__pycache__/
*.pyc
venv/
.DS_Store
docs/_site/
```

---

## Git-Aliase (optional)

In `~/.gitconfig`:

```ini
[alias]
    st = status
    lg = log --oneline --graph
    co = checkout
    br = branch
```

Dann: `git st`, `git lg`, etc.

---

## Vim + Git

Git verwendet Vim für Commit-Messages:

```
i                → Insert-Modus
[Message tippen] → z.B. "fix: Serial-Port korrigiert"
Esc → :wq        → Commit ausführen
Esc → :q!        → Commit abbrechen
```

**Editor ändern:** `git config --global core.editor "code --wait"`

---

*Stand: Januar 2026*
