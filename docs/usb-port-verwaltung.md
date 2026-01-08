# USB-Port-Verwaltung: Selection Panel vs. AMR Platform (Pi 5)

## Ziel

Auf dem Raspberry Pi 5 darf **immer nur ein** Prozess den ESP32-Serial-Port nutzen, sonst gehen Daten verloren oder Reads brechen ab.

**Exklusivität:** gemeinsamer Lock via `flock` auf
`/var/lock/esp32-serial.lock`

- **Selection Panel (systemd):** `flock -n` → startet nur, wenn Lock frei ist.
- **AMR micro-ROS Agent (Docker):** `flock` (ohne `-n`) → wartet, bis Lock frei ist.

## Komponenten

| Projekt | Prozess | Startart | Serial-Port |
|---------|---------|----------|-------------|
| Selection Panel | `server.py` | `systemd` (`selection-panel.service`) | by-id (stabil) |
| AMR Platform | `micro_ros_agent` | Docker Compose (`microros_agent`) | by-id (empfohlen) |

**Stabiler Device-Pfad (empfohlen):**
`/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00`

**Pi-IP (LAN/WLAN):** aus `hostname -I` ist bei dir typ. `192.168.1.24`
(Hinweis: `172.17.0.1` ist Docker-Bridge und i. d. R. nicht für den Browser im LAN gedacht.)

## Nach Reboot: Standard-Ablauf

### A) Selection Panel Modus (UI + Taster/LEDs)

Start:

```bash
sudo systemctl start selection-panel.service
```

Browser:

- `http://rover.local:8080/`
- Fallback (falls mDNS zickt):

  ```bash
  hostname -I
  ```

  dann `http://192.168.1.24:8080/` (bzw. erste LAN/WLAN-IP)

Log/Status:

```bash
sudo systemctl status selection-panel.service --no-pager
sudo journalctl -u selection-panel.service -f
```

Erwartung:

- Taster 1–10 drücken → Logs zeigen `Button X gedrueckt`

### B) AMR Modus (micro-ROS Agent)

1. Selection Panel sauber stoppen (gibt Lock + Port frei):

   ```bash
   sudo systemctl stop selection-panel.service
   ```

2. Agent starten:

   ```bash
   cd /home/pi/amr/docker
   sudo docker compose -p docker up -d microros_agent
   ```

Logs:

```bash
cd /home/pi/amr/docker
sudo docker compose -p docker logs -f microros_agent
```

Erwartung:

- Agent startet und verbindet sich (keine Port-Konflikte)
- Falls Selection Panel noch läuft, wartet der Agent (Lock) statt den Port zu blockieren.

## Schneller Wechsel (ohne Reboot)

### → Selection Panel

```bash
cd /home/pi/amr/docker
sudo docker compose -p docker stop microros_agent

sudo systemctl start selection-panel.service
sudo journalctl -u selection-panel.service -f
```

### → AMR

```bash
sudo systemctl stop selection-panel.service

cd /home/pi/amr/docker
sudo docker compose -p docker up -d microros_agent
sudo docker compose -p docker logs -f microros_agent
```

## Autostart (optional, einmalig)

### Selection Panel beim Boot automatisch starten

```bash
sudo systemctl enable selection-panel.service
```

### Selection Panel Autostart aus

```bash
sudo systemctl disable selection-panel.service
```

> Empfehlung: AMR bewusst per `docker compose up -d microros_agent` starten, wenn du in den AMR-Modus wechselst.
> (Damit ist klar definiert, wer den Port belegt.)

## Kontrolle: Port + Lock (Sanity Checks)

### Wer hält den USB-Port?

```bash
sudo fuser -v /dev/ttyACM0 || true
```

### Wer hält den Lock?

```bash
sudo lslocks | grep esp32-serial || true
ls -l /var/lock/esp32-serial.lock || true
```

## One-time Setup: AMR `docker-compose.yml` mit Serial-Lock

Pfad: `/home/pi/amr/docker/docker-compose.yml`

### Backup

```bash
cd /home/pi/amr/docker
cp -a docker-compose.yml docker-compose.yml.bak.$(date +%F_%H%M%S)
```

### `microros_agent` Lock-Wrapper (empfohlen: by-id)

Passe **nur** den Service `microros_agent` an:

```yaml
services:
  microros_agent:
    image: microros/micro-ros-agent:humble
    container_name: amr_agent
    network_mode: host
    privileged: true
    restart: always

    volumes:
      - /dev:/dev
      - /var/lock:/var/lock

    entrypoint: ["/bin/sh", "-lc"]
    command: >
      DEV="/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00";
      echo "[microros_agent] waiting for /var/lock/esp32-serial.lock (DEV=$DEV)";
      exec flock /var/lock/esp32-serial.lock
      /bin/sh /micro-ros_entrypoint.sh serial --dev "$DEV" -b 921600
```

### Anwenden

```bash
cd /home/pi/amr/docker
sudo docker compose -p docker config >/dev/null
sudo docker compose -p docker up -d --force-recreate microros_agent
```

## Troubleshooting

### 1) Selection Panel startet nicht

Häufig: Lock belegt (AMR hält ihn) → Selection Panel startet absichtlich nicht.

```bash
sudo journalctl -u selection-panel.service -n 120 --no-pager
sudo lslocks | grep esp32-serial || true
sudo fuser -v /dev/ttyACM0 || true
```

Fix:

- Für Selection Panel: `sudo docker compose -p docker stop microros_agent`
- Für AMR: `sudo systemctl stop selection-panel.service`

### 2) Device/Port geändert

```bash
ls -l /dev/serial/by-id/
ls /dev/ttyACM*
```

Fix:

- by-id String im Selection Panel (`server.py`) anpassen
- und/oder `DEV="..."` im Compose-Command anpassen

### 3) Docker/Compose Status

```bash
cd /home/pi/amr/docker
sudo docker compose -p docker ps
sudo docker compose -p docker logs --tail=120 microros_agent
```
