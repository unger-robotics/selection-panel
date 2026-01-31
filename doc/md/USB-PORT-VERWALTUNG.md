# USB-Port- & Port-Management: 3 Projekte auf Pi 5 (finaler Stand)

## Ziel

* **Serial-Port** (ESP32) darf immer nur von **einem** Prozess genutzt werden.
* **HTTP-Ports** müssen eindeutig sein (sonst `Errno 98`).

## SSOT: Serial + Lock

**Device (stabil):**

```bash
/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00
```

**Lockfile:**

```bash
/var/lock/esp32-serial.lock
```

Hinweis: Bei dir ist `/var/lock` ein Symlink auf `/run/lock` (normal).

## Projekte (Serial + HTTP)

| Projekt                 | Prozess                   | Start          | Serial-Lock           | HTTP   |
|-------------------------|---------------------------|----------------|-----------------------|--------|
| Selection Panel         | `selection-panel.service` | systemd        | `flock -n`            | `8080` |
| embedded_projekt Bridge | `embedded-bridge.service` | systemd        | `flock -n`            | `8081` |
| AMR micro-ROS Agent     | `microros_agent`          | Docker Compose | `flock` (blockierend) | —      |

### Boot-Default (bei dir aktuell)

Du hast jetzt:

* `embedded-bridge.service` **enabled**
* `selection-panel.service` **disabled**

Das ist sauber: nach Boot startet standardmäßig **embedded-bridge**.

## Sanity Checks

```bash
# HTTP Listener
sudo ss -ltnp | egrep ':8080|:8081' || true

# Serial-Lock
sudo lslocks | grep esp32-serial || true

# Serial-Port belegt?
sudo fuser -v /dev/ttyACM0 || true
```

Optional: `lsof` installieren (falls du es willst):

```bash
sudo apt update
sudo apt install -y lsof
```

## Switching (ohne Reboot)

### → embedded_projekt (UI :8081)

```bash
# AMR aus (falls läuft)
cd /home/pi/amr/docker
sudo docker compose -p docker stop microros_agent

# Selection Panel aus (falls läuft)
sudo systemctl stop selection-panel.service

# embedded Bridge an
sudo systemctl start embedded-bridge.service
sudo journalctl -u embedded-bridge.service -f
# Browser: http://rover.local:8081/
```

### → Selection Panel (UI :8080)

```bash
sudo systemctl stop embedded-bridge.service
sudo systemctl start selection-panel.service
sudo journalctl -u selection-panel.service -f
# Browser: http://rover.local:8080/
```

### → AMR micro-ROS Agent

```bash
sudo systemctl stop embedded-bridge.service
sudo systemctl stop selection-panel.service

cd /home/pi/amr/docker
sudo docker compose -p docker up -d microros_agent
sudo docker compose -p docker logs -f microros_agent
```

## Boot-Modus umstellen (einmalig)

### Default = embedded_projekt

```bash
sudo systemctl disable selection-panel.service
sudo systemctl enable embedded-bridge.service
```

### Default = selection-panel

```bash
sudo systemctl disable embedded-bridge.service
sudo systemctl enable selection-panel.service
```

