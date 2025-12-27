/**
 * Auswahlpanel Dashboard Client v2.2.4
 * =====================================
 *
 * WebSocket-Client für das interaktive Auswahlpanel.
 * Farbschema: Arduino Teal + Raspberry Pi Red (konsistent mit farbschema.tex)
 * Stand: 2025-12-27
 *
 * Protokoll:
 * - Empfängt: {"type": "stop"} und {"type": "play", "id": n}
 * - Sendet:    {"type": "ended", "id": n}
 */

// =============================================================================
// KONFIGURATION
// =============================================================================

const CONFIG = {
    wsUrl: `ws://${window.location.host}/ws`,
    reconnectInterval: 5000,
    debug: true
};

// =============================================================================
// GLOBALER ZUSTAND
// =============================================================================

const state = {
    ws: null,
    audioUnlocked: false,
    currentId: null,
    isPlaying: false
};

// =============================================================================
// DOM-ELEMENTE
// =============================================================================

const elements = {
    unlockBtn: document.getElementById('unlock-btn'),
    waiting: document.getElementById('waiting'),
    mediaContainer: document.getElementById('media-container'),
    currentId: document.getElementById('current-id'),
    imageContainer: document.getElementById('image-container'),
    audio: document.getElementById('audio'),
    progressFill: document.getElementById('progress-fill'),
    timeDisplay: document.getElementById('time-display'),
    wsStatus: document.getElementById('ws-status'),
    audioStatus: document.getElementById('audio-status'),
    debug: document.getElementById('debug'),
    debugToggle: document.getElementById('debug-toggle')
};

// =============================================================================
// DEBUG-LOGGING
// =============================================================================

function log(message, level = 'info') {
    const timestamp = new Date().toLocaleTimeString();
    const logLine = `[${timestamp}] ${message}`;

    if (CONFIG.debug) {
        console.log(logLine);

        const debugEl = elements.debug;
        debugEl.innerHTML = logLine + '<br>' + debugEl.innerHTML;

        const lines = debugEl.innerHTML.split('<br>');
        if (lines.length > 50) {
            debugEl.innerHTML = lines.slice(0, 50).join('<br>');
        }
    }
}

// =============================================================================
// WEBSOCKET
// =============================================================================

function connectWebSocket() {
    log(`Verbinde mit ${CONFIG.wsUrl}...`);

    try {
        state.ws = new WebSocket(CONFIG.wsUrl);

        state.ws.onopen = () => {
            log('WebSocket verbunden');
            elements.wsStatus.classList.add('connected');
            elements.wsStatus.classList.remove('disconnected');
        };

        state.ws.onclose = () => {
            log('WebSocket getrennt');
            elements.wsStatus.classList.remove('connected');
            elements.wsStatus.classList.add('disconnected');
            setTimeout(connectWebSocket, CONFIG.reconnectInterval);
        };

        state.ws.onerror = (error) => {
            log(`WebSocket-Fehler: ${error.message || 'unbekannt'}`, 'error');
        };

        state.ws.onmessage = (event) => {
            handleServerMessage(event.data);
        };

    } catch (error) {
        log(`WebSocket-Fehler: ${error.message}`, 'error');
        setTimeout(connectWebSocket, CONFIG.reconnectInterval);
    }
}

function sendMessage(data) {
    if (state.ws && state.ws.readyState === WebSocket.OPEN) {
        state.ws.send(JSON.stringify(data));
        log(`TX: ${JSON.stringify(data)}`);
    } else {
        log('WebSocket nicht verbunden', 'warn');
    }
}

// =============================================================================
// NACHRICHTEN-HANDLER
// =============================================================================

function handleServerMessage(data) {
    log(`RX: ${data}`);

    try {
        const message = JSON.parse(data);

        switch (message.type) {
            case 'stop':
                handleStop();
                break;

            case 'play':
                if (typeof message.id === 'number') {
                    handlePlay(message.id);
                } else {
                    log('Ungueltige play-Nachricht', 'warn');
                }
                break;

            default:
                log(`Unbekannter Typ: ${message.type}`, 'warn');
        }

    } catch (error) {
        log(`JSON-Fehler: ${error.message}`, 'error');
    }
}

// =============================================================================
// PLAYBACK-STEUERUNG
// =============================================================================

function handleStop() {
    log('STOP');

    const audio = elements.audio;
    audio.pause();
    audio.currentTime = 0;

    state.isPlaying = false;
    elements.progressFill.style.width = '0%';
}

function handlePlay(id) {
    log(`PLAY: ${id}`);

    if (!state.audioUnlocked) {
        log('Audio nicht entsperrt!', 'warn');
        return;
    }

    state.currentId = id;
    state.isPlaying = true;

    const id3 = id.toString().padStart(3, '0');

    elements.currentId.textContent = id3;
    elements.mediaContainer.classList.add('active');
    elements.waiting.classList.add('hidden');

    // Bild laden
    const imageUrl = `/media/${id3}.jpg`;
    elements.imageContainer.innerHTML = `<img src="${imageUrl}" alt="${id3}" onerror="this.parentElement.innerHTML='<span class=\\'placeholder\\'>Bild nicht gefunden</span>'">`;

    // Audio laden und abspielen
    const audio = elements.audio;
    audio.src = `/media/${id3}.mp3`;
    audio.load();

    audio.play()
        .then(() => log(`Audio gestartet: ${id3}`))
        .catch((e) => log(`Audio-Fehler: ${e.message}`, 'error'));
}

function handleAudioEnded() {
    log(`Audio beendet: ${state.currentId}`);

    if (state.currentId !== null) {
        sendMessage({ type: 'ended', id: state.currentId });
    }

    state.isPlaying = false;
    elements.currentId.textContent = '---';
    elements.progressFill.style.width = '0%';
    elements.timeDisplay.textContent = '0:00 / 0:00';
}

// =============================================================================
// AUDIO-UNLOCK (Browser Autoplay Policy)
// =============================================================================

function unlockAudio() {
    log('Audio-Unlock...');

    const audio = elements.audio;
    audio.volume = 0;
    audio.src = 'data:audio/wav;base64,UklGRiQAAABXQVZFZm10IBAAAAABAAEARKwAAIhYAQACABAAZGF0YQAAAAA=';

    audio.play()
        .then(() => {
            audio.pause();
            audio.volume = 1;
            audio.src = '';

            state.audioUnlocked = true;
            log('Audio entsperrt');

            elements.unlockBtn.classList.add('hidden');
            elements.waiting.classList.remove('hidden');
            elements.audioStatus.classList.remove('disconnected');
            elements.audioStatus.classList.add('connected');
        })
        .catch((e) => {
            log(`Unlock fehlgeschlagen: ${e.message}`, 'error');
            elements.audioStatus.classList.add('disconnected');
        });
}

// =============================================================================
// FORTSCHRITTSANZEIGE
// =============================================================================

function updateProgress() {
    const audio = elements.audio;

    if (!audio.duration || !isFinite(audio.duration)) return;

    const progress = (audio.currentTime / audio.duration) * 100;
    elements.progressFill.style.width = `${progress}%`;

    elements.timeDisplay.textContent = `${formatTime(audio.currentTime)} / ${formatTime(audio.duration)}`;
}

function formatTime(seconds) {
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
}

// =============================================================================
// EVENT-LISTENER
// =============================================================================

function setupEventListeners() {
    // Unlock-Button
    elements.unlockBtn.addEventListener('click', unlockAudio);

    // Audio-Events
    elements.audio.addEventListener('ended', handleAudioEnded);
    elements.audio.addEventListener('timeupdate', updateProgress);
    elements.audio.addEventListener('error', (e) => {
        log(`Audio-Fehler: ${e.target.error?.message || 'unbekannt'}`, 'error');
    });

    // Debug-Toggle
    elements.debugToggle.addEventListener('click', () => {
        const isVisible = elements.debug.classList.toggle('visible');
        elements.debugToggle.textContent = isVisible ? 'Debug [X]' : 'Debug';
        elements.debugToggle.classList.toggle('active', isVisible);
    });

    // Keyboard-Shortcuts
    document.addEventListener('keydown', (e) => {
        // Space: Play/Pause
        if (e.code === 'Space' && state.audioUnlocked) {
            e.preventDefault();
            const audio = elements.audio;
            audio.paused ? audio.play() : audio.pause();
        }

        // Ctrl+D: Debug-Panel
        if (e.code === 'KeyD' && e.ctrlKey) {
            e.preventDefault();
            const isVisible = elements.debug.classList.toggle('visible');
            elements.debugToggle.textContent = isVisible ? 'Debug [X]' : 'Debug';
            elements.debugToggle.classList.toggle('active', isVisible);
        }
    });
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

function init() {
    log('Dashboard v2.2.4 wird initialisiert...');

    setupEventListeners();

    elements.wsStatus.classList.add('disconnected');
    elements.audioStatus.classList.add('disconnected');

    connectWebSocket();

    log('Dashboard bereit');
}

document.addEventListener('DOMContentLoaded', init);
