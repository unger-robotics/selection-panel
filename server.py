#!/usr/bin/env python3
"""
Auswahlpanel Server v2.5.1
==========================

Verbindet ESP32-S3 (Serial) mit Web-Clients (WebSocket).

CHANGELOG v2.5.1:
- NEU: Robustere Serial-Fragment-Erkennung mit Pending-Buffer
- NEU: Timeout-basierte Fragment-Vervollständigung (50ms)
- FIX: USB-CDC Fragmentierung wird jetzt korrekt behandelt

CHANGELOG v2.4.2:
- NEU: Parallele Ausführung von Serial + WebSocket (asyncio.gather)
- NEU: LEDSET wird nur gesendet wenn ESP32 LED nicht selbst gesetzt hat
- Optimiert: Minimale Latenz durch parallele Broadcasts

Policy: "Umschalten gewinnt" (Preempt)
- Jeder neue Tastendruck stoppt sofort und startet neue ID
- One-hot LED: immer nur eine LED aktiv

Serial-Protokoll (1-basiert!):
- ESP32 sendet: PRESS 001 ... PRESS 100
- Server sendet: LEDSET 001 ... LEDSET 100 (optional, ESP32 setzt LED selbst)

Medien (1-basiert!):
- Medien-IDs: 1-100 (für 001.jpg, 001.mp3, etc.)

Autor: Jan Unger
Datum: 2025-01-07
"""

import asyncio
import json
import logging
import sys
import threading
import time
from pathlib import Path
from typing import Optional

from aiohttp import web, WSMsgType

# =============================================================================
# KONFIGURATION
# =============================================================================

# Build-Modus: Anzahl der erwarteten Medien
PROTOTYPE_MODE = True  # True = 10 Medien, False = 100 Medien
NUM_MEDIA = 10 if PROTOTYPE_MODE else 100

# Serial-Port zum ESP32
# Port wechselt zwischen ttyACM0 und ttyACM1
SERIAL_PORT = "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_98:3D:AE:EA:08:1C-if00"
SERIAL_BAUD = 115200

# HTTP-Server
HTTP_HOST = "0.0.0.0"
HTTP_PORT = 8080

# Pfade
BASE_DIR = Path(__file__).parent
STATIC_DIR = BASE_DIR / "static"
MEDIA_DIR = BASE_DIR / "media"

# Logging
LOG_LEVEL = logging.INFO
LOG_FORMAT = "%(asctime)s [%(levelname)s] %(message)s"

# Medien-Validierung
VALIDATE_MEDIA_ON_STARTUP = True
REQUIRED_EXTENSIONS = [".jpg", ".mp3"]

# Optimierung: ESP32 v2.4.1 setzt LED selbst, Server muss nicht senden
ESP32_SETS_LED_LOCALLY = True

# Fragment-Timeout (ms): Warte auf Rest der Zeile bevor Fragment verarbeitet wird
FRAGMENT_TIMEOUT_MS = 50

# =============================================================================
# GLOBALER ZUSTAND
# =============================================================================

class AppState:
    """Zentrale Zustandsverwaltung."""

    def __init__(self):
        self.current_id: Optional[int] = None  # 1-basiert (1-100)
        self.ws_clients: set[web.WebSocketResponse] = set()
        self.serial_fd: Optional[int] = None  # File descriptor für Serial
        self.serial_connected: bool = False
        self.serial_lock = threading.Lock()
        self.media_valid: dict[int, dict[str, bool]] = {}
        self.missing_media: list[str] = []

    async def broadcast(self, message: dict) -> None:
        """Sendet Nachricht an alle WebSocket-Clients."""
        if not self.ws_clients:
            return

        data = json.dumps(message)
        dead_clients = set()

        # Parallel an alle Clients senden
        async def send_to_client(ws):
            try:
                await ws.send_str(data)
            except Exception as e:
                logging.warning(f"WebSocket-Fehler: {e}")
                dead_clients.add(ws)

        await asyncio.gather(*[send_to_client(ws) for ws in self.ws_clients])
        self.ws_clients -= dead_clients

    async def send_serial(self, command: str) -> bool:
        """Sendet Befehl an ESP32."""
        import os

        if self.serial_fd is None or not self.serial_connected:
            logging.warning(f"Serial nicht verbunden: {command}")
            return False

        try:
            with self.serial_lock:
                os.write(self.serial_fd, f"{command}\n".encode())
            logging.debug(f"Serial TX: {command}")
            return True
        except Exception as e:
            logging.error(f"Serial-Fehler: {e}")
            self.serial_connected = False
            return False


state = AppState()

# =============================================================================
# MEDIEN-VALIDIERUNG
# =============================================================================

def validate_media() -> tuple[bool, list[str]]:
    """
    Prueft ob alle erwarteten Medien-Dateien vorhanden sind.
    Medien sind 1-basiert: 001.jpg - 010.jpg (bzw. 100.jpg)

    Returns:
        (all_valid, missing_files)
    """
    missing = []
    valid_count = 0

    for i in range(1, NUM_MEDIA + 1):  # 1 bis NUM_MEDIA
        media_id = f"{i:03d}"
        id_valid = True

        for ext in REQUIRED_EXTENSIONS:
            filepath = MEDIA_DIR / f"{media_id}{ext}"

            if not filepath.exists():
                missing.append(f"{media_id}{ext}")
                id_valid = False
            elif filepath.stat().st_size == 0:
                missing.append(f"{media_id}{ext} (leer)")
                id_valid = False

        state.media_valid[i] = {
            "jpg": (MEDIA_DIR / f"{media_id}.jpg").exists(),
            "mp3": (MEDIA_DIR / f"{media_id}.mp3").exists(),
        }

        if id_valid:
            valid_count += 1

    state.missing_media = missing
    all_valid = len(missing) == 0

    logging.info(f"Medien-Validierung: {valid_count}/{NUM_MEDIA} vollstaendig")
    if missing:
        logging.warning(f"Fehlende Medien: {len(missing)} Dateien")
        for m in missing[:10]:  # Nur erste 10 anzeigen
            logging.warning(f"  - {m}")
        if len(missing) > 10:
            logging.warning(f"  ... und {len(missing) - 10} weitere")

    return all_valid, missing


def check_media_exists(media_id: int) -> dict:
    """Prueft ob Medien fuer eine ID existieren (1-basiert)."""
    if media_id < 1 or media_id > NUM_MEDIA:
        return {"valid": False, "jpg": False, "mp3": False}

    return state.media_valid.get(media_id, {"jpg": False, "mp3": False})

# =============================================================================
# SERIAL-HANDLER
# =============================================================================

async def handle_serial_line(line: str) -> None:
    """Verarbeitet eine Zeile vom ESP32."""
    line = line.strip()
    if not line:
        return

    logging.debug(f"Serial RX: '{line}'")

    # PRESS erkennen (vollständig)
    if line.startswith("PRESS "):
        button_id = parse_button_id(line[6:])
        if button_id:
            await handle_button_press(button_id)
            return

    # Fallback: Fragmentiertes PRESS (nur Zahl ohne "PRESS " Prefix)
    # USB-CDC kann "PRESS " und "003" als separate Zeilen senden
    if not line.startswith("RE") and not line.startswith("SE"):
        button_id = parse_button_id(line)
        if button_id:
            logging.debug(f"Fragmentiertes PRESS: '{line}' -> Button {button_id}")
            await handle_button_press(button_id)
            return

    # RELEASE erkennen
    if line.startswith("RELEASE "):
        logging.debug(f"Button released: {line[8:]}")
        return

    # Standard-Befehle
    if line == "READY":
        logging.info("ESP32 ist bereit")
        state.serial_connected = True

    elif line == "PONG":
        logging.debug("PING-Antwort erhalten")

    elif line.startswith("OK"):
        logging.debug(f"ESP32: {line}")

    elif line.startswith("ERROR "):
        logging.warning(f"ESP32 Fehler: {line[6:]}")

    elif line.startswith("FW "):
        logging.info(f"ESP32 Firmware: {line}")

    elif line.startswith("MODE "):
        logging.info(f"ESP32 Modus: {line[5:]}")

    elif line.startswith("CURLED ") or line.startswith("BTNS ") or line.startswith("LEDS ") or line.startswith("HEAP "):
        logging.debug(f"ESP32 Status: {line}")


def parse_button_id(s: str) -> int | None:
    """Extrahiert Button-ID aus String (1-NUM_MEDIA). Gibt None zurück bei Fehler."""
    s = s.strip()
    if not s:
        return None

    # Nur Ziffern erlaubt
    if not s.isdigit():
        return None

    try:
        button_id = int(s)
        if 1 <= button_id <= NUM_MEDIA:
            return button_id
    except ValueError:
        pass
    return None


async def handle_button_press(button_id: int) -> None:
    """
    Verarbeitet Tastendruck (Preempt-Policy) mit minimaler Latenz.

    Args:
        button_id: 1-basierte ID (Taster 1-100, Medien 001-100)
    """
    # ID-Validierung (1-basiert)
    if button_id < 1 or button_id > NUM_MEDIA:
        logging.warning(f"Button-ID ausserhalb Bereich: {button_id} (erlaubt: 1-{NUM_MEDIA})")
        return

    # Medien-Validierung
    media_status = check_media_exists(button_id)
    if not media_status.get("jpg") or not media_status.get("mp3"):
        logging.warning(f"Medien fuer ID {button_id} unvollstaendig: {media_status}")
        # Trotzdem fortfahren - Browser zeigt Fehlermeldung

    logging.info(f"Button {button_id} gedrueckt")

    state.current_id = button_id

    # =========================================================================
    # OPTIMIERT v2.4.2: Alle Aktionen PARALLEL ausführen
    # =========================================================================

    tasks = []

    # 1. ESP32: LED setzen (optional - ESP32 v2.4.1 macht das selbst)
    if not ESP32_SETS_LED_LOCALLY:
        tasks.append(state.send_serial(f"LEDSET {button_id:03d}"))

    # 2. Browser: Stop-Signal (alle laufenden Medien stoppen)
    tasks.append(state.broadcast({"type": "stop"}))

    # 3. Browser: Play-Signal (neue Medien starten)
    tasks.append(state.broadcast({"type": "play", "id": button_id}))

    # Alle Tasks parallel ausführen
    if tasks:
        await asyncio.gather(*tasks)


async def handle_playback_ended(ended_id: int) -> None:
    """
    Verarbeitet Wiedergabe-Ende mit Race-Condition-Schutz.

    Args:
        ended_id: 1-basierte ID
    """
    logging.debug(f"Playback beendet: {ended_id}, current: {state.current_id}")

    if state.current_id == ended_id:
        logging.info(f"Wiedergabe {ended_id} beendet -> LEDs aus")
        state.current_id = None
        await state.send_serial("LEDCLR")
    else:
        logging.debug(f"Race-Condition vermieden: ended={ended_id}, current={state.current_id}")


async def serial_reader_task() -> None:
    """Liest kontinuierlich vom Serial-Port mit Reconnect (Thread-basiert)."""
    import os
    import subprocess
    import select

    loop = asyncio.get_event_loop()

    def serial_thread():
        """Blocking Serial-Reader in separatem Thread mit robustem Parsing."""
        while True:
            fd_read = None
            fd_write = None

            try:
                # Port konfigurieren mit stty
                subprocess.run(
                    ["stty", "-F", SERIAL_PORT, str(SERIAL_BAUD), "raw", "-echo"],
                    check=True,
                    capture_output=True
                )

                # Port öffnen (separate FDs für Lesen und Schreiben)
                fd_read = os.open(SERIAL_PORT, os.O_RDONLY | os.O_NONBLOCK)
                fd_write = os.open(SERIAL_PORT, os.O_WRONLY)

                state.serial_fd = fd_write
                state.serial_connected = True
                logging.info(f"Serial verbunden: {SERIAL_PORT}")

                # ============================================================
                # Robuster Ringpuffer mit Fragment-Handling
                # ============================================================
                buffer = b""
                pending_fragment = ""
                last_data_time = 0
                poll = select.poll()
                poll.register(fd_read, select.POLLIN)

                while state.serial_connected:
                    try:
                        events = poll.poll(20)  # 20ms Timeout für responsive Fragment-Handling
                        current_time = time.time() * 1000  # ms

                        for fd, event in events:
                            if event & select.POLLIN:
                                try:
                                    data = os.read(fd_read, 1024)
                                    if data:
                                        buffer += data
                                        last_data_time = current_time

                                        # Komplette Zeilen verarbeiten
                                        while b'\n' in buffer:
                                            line, buffer = buffer.split(b'\n', 1)
                                            line_str = line.decode('utf-8', errors='replace').strip()

                                            if line_str:
                                                # Prüfen ob pending fragment dazugehört
                                                if pending_fragment:
                                                    # Fragment + aktuelle Zeile zusammenfügen
                                                    combined = pending_fragment + line_str
                                                    logging.debug(f"Fragment kombiniert: '{pending_fragment}' + '{line_str}' = '{combined}'")
                                                    pending_fragment = ""

                                                    # Prüfen ob kombiniert ein PRESS ist
                                                    if combined.startswith("PRESS "):
                                                        asyncio.run_coroutine_threadsafe(
                                                            handle_serial_line(combined),
                                                            loop
                                                        )
                                                        continue

                                                # Normale Zeile verarbeiten
                                                asyncio.run_coroutine_threadsafe(
                                                    handle_serial_line(line_str),
                                                    loop
                                                )

                                except BlockingIOError:
                                    pass

                        # Fragment-Timeout: Wenn Buffer Daten hat aber kein \n kam
                        if buffer and (current_time - last_data_time) > FRAGMENT_TIMEOUT_MS:
                            # Buffer als Fragment speichern (ohne \n empfangen)
                            fragment = buffer.decode('utf-8', errors='replace').strip()
                            if fragment:
                                # Ist es ein bekanntes Fragment-Muster?
                                if fragment in ("PRESS", "PRES", "PRE", "PR", "P"):
                                    # Anfang eines PRESS - als pending speichern
                                    pending_fragment = fragment + " "
                                    logging.debug(f"Pending fragment gespeichert: '{fragment}'")
                                elif fragment.startswith("PRESS"):
                                    # Unvollständiges PRESS (z.B. "PRESS 0")
                                    pending_fragment = fragment
                                    logging.debug(f"Incomplete PRESS fragment: '{fragment}'")
                                else:
                                    # Unbekanntes Fragment - versuche zu verarbeiten
                                    logging.debug(f"Fragment-Timeout: '{fragment}'")
                                    # Prüfen ob es eine reine Zahl ist (fragmentiertes PRESS)
                                    button_id = parse_button_id(fragment)
                                    if button_id:
                                        logging.debug(f"Fragmentiertes PRESS erkannt: '{fragment}' -> {button_id}")
                                        asyncio.run_coroutine_threadsafe(
                                            handle_button_press(button_id),
                                            loop
                                        )
                            buffer = b""

                    except OSError as e:
                        logging.error(f"Serial-Lesefehler: {e}")
                        break

            except FileNotFoundError:
                logging.error(f"Serial-Port nicht gefunden: {SERIAL_PORT}")
            except PermissionError:
                logging.error(f"Keine Berechtigung fuer {SERIAL_PORT}")
            except subprocess.CalledProcessError as e:
                logging.error(f"stty Fehler: {e}")
            except Exception as e:
                logging.error(f"Unerwarteter Fehler: {e}")
            finally:
                state.serial_connected = False
                state.serial_fd = None
                if fd_read is not None:
                    try:
                        os.close(fd_read)
                    except:
                        pass
                if fd_write is not None:
                    try:
                        os.close(fd_write)
                    except:
                        pass

            logging.info("Serial Reconnect in 5s...")
            time.sleep(5)

    # Thread starten
    thread = threading.Thread(target=serial_thread, daemon=True)
    thread.start()

    # Task am Leben halten
    while True:
        await asyncio.sleep(1)

# =============================================================================
# WEBSOCKET-HANDLER
# =============================================================================

async def websocket_handler(request: web.Request) -> web.WebSocketResponse:
    """WebSocket-Endpoint fuer Browser-Clients."""
    ws = web.WebSocketResponse()
    await ws.prepare(request)

    state.ws_clients.add(ws)
    client_ip = request.remote
    logging.info(f"WS-Client verbunden: {client_ip} (Total: {len(state.ws_clients)})")

    try:
        async for msg in ws:
            if msg.type == WSMsgType.TEXT:
                try:
                    data = json.loads(msg.data)
                    await handle_ws_message(data)
                except json.JSONDecodeError as e:
                    logging.warning(f"Ungueltiges JSON von {client_ip}: {msg.data[:50]}")
            elif msg.type == WSMsgType.ERROR:
                logging.error(f"WS-Fehler: {ws.exception()}")
    except Exception as e:
        logging.error(f"WS-Exception: {e}")
    finally:
        state.ws_clients.discard(ws)
        logging.info(f"WS-Client getrennt: {client_ip} (Total: {len(state.ws_clients)})")

    return ws


async def handle_ws_message(data: dict) -> None:
    """Verarbeitet WebSocket-Nachricht."""
    msg_type = data.get("type")

    if msg_type == "ended":
        # Browser sendet 1-basierte ID
        ended_id = data.get("id")
        if isinstance(ended_id, int):
            await handle_playback_ended(ended_id)
        else:
            logging.warning(f"'ended' ohne gueltige ID: {data}")

    elif msg_type == "ping":
        # Heartbeat von Client
        pass

    else:
        logging.debug(f"Unbekannter WS-Nachrichtentyp: {msg_type}")

# =============================================================================
# HTTP-HANDLER
# =============================================================================

async def index_handler(request: web.Request) -> web.FileResponse:
    """Liefert die Hauptseite."""
    return web.FileResponse(STATIC_DIR / "index.html")


async def test_play_handler(request: web.Request) -> web.Response:
    """Test-Endpoint: Simuliert Tastendruck (1-basierte ID)."""
    try:
        test_id = int(request.match_info["id"])
        if 1 <= test_id <= NUM_MEDIA:
            await handle_button_press(test_id)
            return web.Response(text=f"OK: play {test_id}")
        return web.Response(text=f"ERROR: ID muss 1-{NUM_MEDIA} sein", status=400)
    except ValueError:
        return web.Response(text="ERROR: Ungueltige ID", status=400)


async def test_stop_handler(request: web.Request) -> web.Response:
    """Test-Endpoint: Stoppt Wiedergabe."""
    state.current_id = None
    await state.broadcast({"type": "stop"})
    await state.send_serial("LEDCLR")
    return web.Response(text="OK: stopped")


async def status_handler(request: web.Request) -> web.Response:
    """Server-Status als JSON."""
    return web.json_response({
        "version": "2.5.1",
        "mode": "prototype" if PROTOTYPE_MODE else "production",
        "num_media": NUM_MEDIA,
        "current_button": state.current_id,  # 1-basiert
        "ws_clients": len(state.ws_clients),
        "serial_connected": state.serial_connected,
        "serial_port": SERIAL_PORT,
        "media_missing": len(state.missing_media),
        "missing_files": state.missing_media[:10] if state.missing_media else [],
        "esp32_local_led": ESP32_SETS_LED_LOCALLY,
    })


async def health_handler(request: web.Request) -> web.Response:
    """Health-Check fuer Monitoring."""
    healthy = state.serial_connected and len(state.missing_media) == 0

    return web.json_response(
        {
            "status": "healthy" if healthy else "degraded",
            "serial": state.serial_connected,
            "media_complete": len(state.missing_media) == 0,
        },
        status=200 if healthy else 503
    )

# =============================================================================
# APP-SETUP
# =============================================================================

def create_app() -> web.Application:
    """Erstellt die aiohttp-Application."""
    app = web.Application()

    app.router.add_get("/", index_handler)
    app.router.add_get("/ws", websocket_handler)
    app.router.add_get("/status", status_handler)
    app.router.add_get("/health", health_handler)
    app.router.add_get("/test/play/{id}", test_play_handler)
    app.router.add_get("/test/stop", test_stop_handler)

    if STATIC_DIR.exists():
        app.router.add_static("/static/", STATIC_DIR)
    else:
        logging.warning(f"Static-Verzeichnis fehlt: {STATIC_DIR}")

    if MEDIA_DIR.exists():
        app.router.add_static("/media/", MEDIA_DIR)
    else:
        logging.warning(f"Media-Verzeichnis fehlt: {MEDIA_DIR}")

    return app


async def start_background_tasks(app: web.Application) -> None:
    app["serial_task"] = asyncio.create_task(serial_reader_task())


async def cleanup_background_tasks(app: web.Application) -> None:
    app["serial_task"].cancel()
    try:
        await app["serial_task"]
    except asyncio.CancelledError:
        pass

# =============================================================================
# MAIN
# =============================================================================

def main() -> None:
    logging.basicConfig(level=LOG_LEVEL, format=LOG_FORMAT)

    mode_str = "PROTOTYPE" if PROTOTYPE_MODE else "PRODUCTION"

    logging.info("=" * 50)
    logging.info(f"Auswahlpanel Server v2.5.1 ({mode_str})")
    logging.info("=" * 50)
    logging.info(f"Medien: {NUM_MEDIA} erwartet (IDs: 001-{NUM_MEDIA:03d})")
    logging.info(f"Taster: 1-{NUM_MEDIA} (1-basiert)")
    logging.info(f"Serial: {SERIAL_PORT}")
    logging.info(f"HTTP:   http://{HTTP_HOST}:{HTTP_PORT}/")
    logging.info(f"ESP32 lokale LED: {'JA' if ESP32_SETS_LED_LOCALLY else 'NEIN'}")
    logging.info("=" * 50)

    # Verzeichnisse erstellen
    STATIC_DIR.mkdir(parents=True, exist_ok=True)
    MEDIA_DIR.mkdir(parents=True, exist_ok=True)

    # Medien validieren
    if VALIDATE_MEDIA_ON_STARTUP:
        all_valid, missing = validate_media()
        if not all_valid:
            logging.warning("WARNUNG: Nicht alle Medien vorhanden!")
            logging.warning("System startet trotzdem - fehlende Medien erzeugen Fehler im Browser")

    logging.info("=" * 50)

    app = create_app()
    app.on_startup.append(start_background_tasks)
    app.on_cleanup.append(cleanup_background_tasks)

    web.run_app(app, host=HTTP_HOST, port=HTTP_PORT, print=None)


if __name__ == "__main__":
    main()
