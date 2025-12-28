# SSH-Setup

Pi einrichten und passwortloses `ssh rover` konfigurieren.

---

## 1. Pi OS installieren

### Raspberry Pi Imager

1. **Download:** [raspberrypi.com/software](https://www.raspberrypi.com/software/)
2. **OS:** Raspberry Pi OS Lite (64-bit)
3. **Einstellungen** (Zahnrad):

| Einstellung | Wert |
|:------------|:-----|
| Hostname | `rover` |
| SSH | ✓ aktivieren |
| Benutzer | `pi` |
| Passwort | (Ihr Passwort) |
| WLAN | SSID + Passwort |
| Zeitzone | `Europe/Berlin` |

4. **Schreiben** → SD-Karte einlegen → Netzteil einstecken

### Nach erstem Boot

```bash
ssh pi@rover.local
sudo apt update && sudo apt upgrade -y
sudo usermod -aG dialout pi
```

---

## 2. SSH-Key einrichten

### Mac / Linux

```bash
ssh-keygen -t ed25519
ssh-copy-id pi@rover.local
```

### Windows (PowerShell)

```powershell
# OpenSSH aktivieren (Admin)
Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0

# Key erstellen
ssh-keygen -t ed25519

# Key kopieren
type $env:USERPROFILE\.ssh\id_ed25519.pub | ssh pi@rover.local "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys && chmod 600 ~/.ssh/authorized_keys"
```

---

## 3. SSH-Config

### Mac / Linux (`~/.ssh/config`)

```
Host rover
    HostName rover.local
    User pi
    IdentityFile ~/.ssh/id_ed25519
```

```bash
chmod 600 ~/.ssh/config
```

### Windows (`%USERPROFILE%\.ssh\config`)

```
Host rover
    HostName rover.local
    User pi
    IdentityFile ~/.ssh/id_ed25519
```

**Test:** `ssh rover`

---

## 4. GitHub (optional)

```bash
ssh-keygen -t ed25519 -f ~/.ssh/id_ed25519_github
```

**~/.ssh/config erweitern:**

```
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_github
    IdentitiesOnly yes
```

Public Key bei GitHub: Settings → SSH Keys → New

```bash
ssh -T git@github.com
```

---

## Troubleshooting

| Problem | Lösung |
|:--------|:-------|
| `Permission denied (publickey)` | `ssh-copy-id` wiederholen |
| `Could not resolve hostname` | IP nutzen: `ssh pi@192.168.x.x` |
| `UNPROTECTED PRIVATE KEY FILE` | `chmod 600 ~/.ssh/id_ed25519` |

---

*Stand: Dezember 2025*
