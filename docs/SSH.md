# SSH-Setup

> Passwortloses `ssh rover` einrichten

---

## 1. Raspberry Pi vorbereiten

### Pi OS installieren (Raspberry Pi Imager)

Erweiterte Optionen im Imager:

| Einstellung | Wert |
|-------------|------|
| Hostname | `rover` |
| SSH | ✓ aktivieren |
| Benutzer | `pi` |
| WLAN | SSID + Passwort |
| Zeitzone | `Europe/Berlin` |

### Nach erstem Boot

```bash
ssh pi@rover.local              # Mit Passwort
sudo apt update && sudo apt upgrade -y
sudo usermod -aG dialout pi     # Für Serial-Zugriff
```

---

## 2. Mac einrichten

```bash
# Key erstellen (falls nicht vorhanden)
ssh-keygen -t ed25519 -C "dein@email.com"

# Key auf Pi kopieren
ssh-copy-id -i ~/.ssh/id_ed25519.pub pi@rover.local

# Config anlegen
vim ~/.ssh/config
```

```
Host rover
    HostName rover.local
    User pi
    IdentityFile ~/.ssh/id_ed25519
```

```bash
# Berechtigungen
chmod 600 ~/.ssh/config ~/.ssh/id_ed25519

# Testen
ssh rover
```

---

## 3. Windows einrichten

### OpenSSH aktivieren (PowerShell als Admin)

```powershell
Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0
```

### Key erstellen (PowerShell normal)

```powershell
ssh-keygen -t ed25519 -C "dein@email.com"
```

### Key auf Pi kopieren

```powershell
type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh pi@rover.local "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys"
```

### Config anlegen

```powershell
notepad $env:USERPROFILE\.ssh\config
```

```
Host rover
    HostName rover.local
    User pi
    IdentityFile ~/.ssh/id_ed25519
```

---

## 4. GitHub (optional)

```bash
# Separater Key
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519_github

# Config erweitern
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_github
    IdentitiesOnly yes
```

Public Key bei GitHub hinterlegen: Settings → SSH Keys → New

```bash
# Testen
ssh -T git@github.com
```

---

## Troubleshooting

| Problem | Lösung |
|---------|--------|
| `Permission denied (publickey)` | `ssh-copy-id` wiederholen |
| `Could not resolve hostname` | IP direkt nutzen oder mDNS prüfen |
| `UNPROTECTED PRIVATE KEY FILE` | `chmod 600 ~/.ssh/id_ed25519` |
