#!/usr/bin/env python3
"""
Auswahlpanel Server v2.2.0
==========================

Verbindet ESP32-S3 (Serial) mit Web-Clients (WebSocket).

Policy: "Umschalten gewinnt" (Preempt)
- Jeder neue Tastendruck stoppt sofort und startet neue ID
- One-hot LED: immer nur eine LED aktiv

Autor: Jan Unger
Datum: 2025-12-26
"""

import asyncio
import json
import logging
import sys
from pathlib import Path
from typing import Optional

from aiohttp import web, WSMsgType
import serial_asyncio

# =============================================================================
# KONFIGURATION
# =============================================================================

# Build-Modus: Anzahl der erwarteten Medien
PROTOTYPE_MODE = True  # True = 10 Medien, False = 100 Medien
NUM_MEDIA = 10 if PROTOTYPE_MODE else 100

# Serial-Port zum ESP32 (ermitteln mit: ls -l /dev/serial/by-id/)
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

# =============================================================================
# GLOBALER ZUSTAND
# =============================================================================

class AppState:
    """Zentrale Zustandsverwaltung."""

    def __init__(self):
        self.current_id: Optional[int] = None
        self.ws_clients: set[web.WebSocketResponse] = set()
        self.serial_writer: Optional[asyncio.StreamWriter] = None
        self.serial_connected: bool = False
        self.media_valid: dict[int, dict[str, bool]] = {}
        self.missing_media: list[str] = []

    async def broadcast(self, message: dict) -> None:
        """Sendet Nachricht an alle WebSocket-Clients."""
        if not self.ws_clients:
            return

        data = json.dumps(message)
        dead_clients = set()

        for ws in self.ws_clients:
            try:
                await ws.send_str(data)
            except Exception as e:
                logging.warning(f"WebSocket-Fehler: {e}")
                dead_clients.add(ws)

        self.ws_clients -= dead_clients

    async def send_serial(self, command: str) -> bool:
        """Sendet Befehl an ESP32."""
        if not self.serial_writer or not self.serial_connected:
            logging.warning(f"Serial nicht verbunden: {command}")
            return False

        try:
            self.serial_writer.write(f"{command}\n".encode())
            await self.serial_writer.drain()
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

    Returns:
        (all_valid, missing_files)
    """
    missing = []
    valid_count = 0

    for i in range(NUM_MEDIA):
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
    """Prueft ob Medien fuer eine ID existieren."""
    if media_id < 0 or media_id >= NUM_MEDIA:
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

    logging.debug(f"Serial RX: {line}")

    if line.startswith("PRESS "):
        try:
            # Robustere Parsing mit Fehlerbehandlung
            parts = line.split()
            if len(parts) >= 2:
                button_id = int(parts[1])
                await handle_button_press(button_id)
            else:
                logging.warning(f"PRESS ohne ID: {line}")
        except ValueError as e:
            logging.warning(f"Ungueltige Button-ID in '{line}': {e}")
        except Exception as e:
            logging.error(f"Fehler bei PRESS-Verarbeitung: {e}")

    elif line.startswith("RELEASE "):
        # Optional: Release-Events loggen
        logging.debug(f"Button released: {line[8:]}")

    elif line == "READY":
        logging.info("ESP32 ist bereit")
        state.serial_connected = True

    elif line == "PONG":
        logging.debug("PING-Antwort erhalten")

    elif line.startswith("ERROR "):
        logging.warning(f"ESP32 Fehler: {line[6:]}")

    elif line.startswith("FW "):
        logging.info(f"ESP32 Firmware: {line}")

    elif line.startswith("MODE "):
        logging.info(f"ESP32 Modus: {line[5:]}")


async def handle_button_press(button_id: int) -> None:
    """Verarbeitet Tastendruck (Preempt-Policy)."""
    # ID-Validierung
    if button_id < 0 or button_id >= NUM_MEDIA:
        logging.warning(f"Button-ID ausserhalb Bereich: {button_id} (max: {NUM_MEDIA - 1})")
        return

    # Medien-Validierung
    media_status = check_media_exists(button_id)
    if not media_status.get("jpg") or not media_status.get("mp3"):
        logging.warning(f"Medien fuer ID {button_id} unvollstaendig: {media_status}")
        # Trotzdem fortfahren - Browser zeigt Fehlermeldung

    logging.info(f"Button {button_id} gedrueckt")

    state.current_id = button_id

    await state.send_serial(f"LEDSET {button_id:03d}")
    await state.broadcast({"type": "stop"})
    await state.broadcast({"type": "play", "id": button_id})


async def handle_playback_ended(ended_id: int) -> None:
    """Verarbeitet Wiedergabe-Ende mit Race-Condition-Schutz."""
    logging.debug(f"Playback beendet: {ended_id}, current: {state.current_id}")

    if state.current_id == ended_id:
        logging.info(f"Wiedergabe {ended_id} beendet -> LEDs aus")
        state.current_id = None
        await state.send_serial("LEDCLR")
    else:
        logging.debug(f"Race-Condition vermieden: ended={ended_id}, current={state.current_id}")


async def serial_reader_task() -> None:
    """Liest kontinuierlich vom Serial-Port mit Reconnect."""
    while True:
        try:
            logging.info(f"Serial verbinde: {SERIAL_PORT}")

            reader, writer = await serial_asyncio.open_serial_connection(
                url=SERIAL_PORT,
                baudrate=SERIAL_BAUD
            )

            state.serial_writer = writer
            state.serial_connected = True
            logging.info("Serial verbunden")

            await state.send_serial("PING")

            while True:
                line = await reader.readline()
                if not line:
                    break
                await handle_serial_line(line.decode('utf-8', errors='replace'))

        except FileNotFoundError:
            logging.error(f"Serial-Port nicht gefunden: {SERIAL_PORT}")
        except PermissionError:
            logging.error(f"Keine Berechtigung fuer {SERIAL_PORT} - 'sudo usermod -aG dialout $USER'")
        except Exception as e:
            logging.error(f"Serial-Fehler: {e}")
        finally:
            state.serial_connected = False
            state.serial_writer = None

        logging.info("Serial Reconnect in 5s...")
        await asyncio.sleep(5)

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
    """Test-Endpoint: Simuliert Tastendruck."""
    try:
        test_id = int(request.match_info["id"])
        if 0 <= test_id < NUM_MEDIA:
            await handle_button_press(test_id)
            return web.Response(text=f"OK: play {test_id}")
        return web.Response(text=f"ERROR: ID muss 0-{NUM_MEDIA - 1} sein", status=400)
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
        "version": "2.2.0",
        "mode": "prototype" if PROTOTYPE_MODE else "production",
        "num_media": NUM_MEDIA,
        "current_id": state.current_id,
        "ws_clients": len(state.ws_clients),
        "serial_connected": state.serial_connected,
        "serial_port": SERIAL_PORT,
        "media_missing": len(state.missing_media),
        "missing_files": state.missing_media[:10] if state.missing_media else [],
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
    logging.info(f"Auswahlpanel Server v2.2.0 ({mode_str})")
    logging.info("=" * 50)
    logging.info(f"Medien: {NUM_MEDIA} erwartet")
    logging.info(f"Serial: {SERIAL_PORT}")
    logging.info(f"HTTP:   http://{HTTP_HOST}:{HTTP_PORT}/")
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
