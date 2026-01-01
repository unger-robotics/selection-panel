/**
 * Auswahlpanel Dashboard Client v2.3.0
 * =====================================
 *
 * WebSocket-Client für das interaktive Auswahlpanel.
 * Farbschema: Arduino Teal + Raspberry Pi Red
 * Stand: 2025-01-01
 *
 * CHANGELOG v2.3.0:
 * - NEU: Medien-Preloading nach Audio-Unlock (instant playback)
 * - NEU: Preload-Statusanzeige
 * - NEU: Gecachte Audio/Image-Objekte für sofortige Wiedergabe
 * - Optimiert: handlePlay() nutzt vorgeladene Medien
 *
 * Protokoll:
 * - Empfängt: {"type": "stop"} und {"type": "play", "id": n}
 * - Sendet:   {"type": "ended", "id": n}
 */

// =============================================================================
// KONFIGURATION
// =============================================================================

const CONFIG = {
    wsUrl: `ws://${window.location.host}/ws`,
    reconnectInterval: 5000,
    debug: true,
    numMedia: 10,  // PROTOTYPE_MODE - auf 100 ändern für Produktion
    preloadConcurrency: 3  // Parallele Preload-Requests
};

// =============================================================================
// GLOBALER ZUSTAND
// =============================================================================

const state = {
    ws: null,
    audioUnlocked: false,
    currentId: null,
    isPlaying: false,
    preloaded: false,
    preloadProgress: 0
};

// Preload-Cache für sofortigen Zugriff
const mediaCache = {
    images: {},   // id -> HTMLImageElement
    audio: {}     // id -> HTMLAudioElement
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
        if (debugEl) {
            debugEl.innerHTML = logLine + '<br>' + debugEl.innerHTML;

            const lines = debugEl.innerHTML.split('<br>');
            if (lines.length > 50) {
                debugEl.innerHTML = lines.slice(0, 50).join('<br>');
            }
        }
    }
}

// =============================================================================
// MEDIEN-PRELOADING (NEU v2.3.0)
// =============================================================================

/**
 * Lädt alle Medien vor für sofortige Wiedergabe.
 * Wird nach Audio-Unlock aufgerufen.
 */
async function preloadAllMedia() {
    log(`Preloading ${CONFIG.numMedia} Medien...`);
    updateWaitingText(`Lade Medien... 0/${CONFIG.numMedia}`);

    const startTime = performance.now();
    let loaded = 0;
    let failed = 0;

    // Medien in Batches laden (begrenzte Parallelität)
    const ids = Array.from({ length: CONFIG.numMedia }, (_, i) => i + 1);

    // Semaphore für begrenzte Parallelität
    const semaphore = new Semaphore(CONFIG.preloadConcurrency);

    const promises = ids.map(async (id) => {
        await semaphore.acquire();
        try {
            await preloadMediaPair(id);
            loaded++;
            state.preloadProgress = Math.round((loaded / CONFIG.numMedia) * 100);
            updateWaitingText(`Lade Medien... ${loaded}/${CONFIG.numMedia}`);
        } catch (e) {
            failed++;
            log(`Preload fehlgeschlagen für ID ${id}: ${e.message}`, 'warn');
        } finally {
            semaphore.release();
        }
    });

    await Promise.all(promises);

    const duration = Math.round(performance.now() - startTime);
    log(`Preload abgeschlossen: ${loaded}/${CONFIG.numMedia} OK, ${failed} Fehler (${duration}ms)`);

    state.preloaded = true;
    updateWaitingText('Warte auf Tastendruck...');

    return { loaded, failed, duration };
}

/**
 * Lädt Bild + Audio für eine ID vor.
 */
async function preloadMediaPair(id) {
    const id3 = id.toString().padStart(3, '0');

    // Bild vorladen
    const imgPromise = new Promise((resolve, reject) => {
        const img = new Image();
        img.onload = () => {
            mediaCache.images[id] = img;
            resolve();
        };
        img.onerror = () => reject(new Error(`Image ${id3}.jpg`));
        img.src = `/media/${id3}.jpg`;
    });

    // Audio vorladen
    const audioPromise = new Promise((resolve, reject) => {
        const audio = new Audio();
        audio.preload = 'auto';

        audio.oncanplaythrough = () => {
            mediaCache.audio[id] = audio;
            resolve();
        };
        audio.onerror = () => reject(new Error(`Audio ${id3}.mp3`));

        // Timeout für langsame Verbindungen
        setTimeout(() => {
            if (!mediaCache.audio[id]) {
                // Auch bei Timeout speichern - besser als nichts
                mediaCache.audio[id] = audio;
                resolve();
            }
        }, 5000);

        audio.src = `/media/${id3}.mp3`;
        audio.load();
    });

    await Promise.all([imgPromise, audioPromise]);
}

/**
 * Einfache Semaphore für begrenzte Parallelität.
 */
class Semaphore {
    constructor(max) {
        this.max = max;
        this.current = 0;
        this.queue = [];
    }

    async acquire() {
        if (this.current < this.max) {
            this.current++;
            return;
        }
        await new Promise(resolve => this.queue.push(resolve));
        this.current++;
    }

    release() {
        this.current--;
        if (this.queue.length > 0) {
            const next = this.queue.shift();
            next();
        }
    }
}

/**
 * Aktualisiert den Waiting-Text.
 */
function updateWaitingText(text) {
    if (elements.waiting) {
        elements.waiting.textContent = text;
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
// PLAYBACK-STEUERUNG (OPTIMIERT v2.3.0)
// =============================================================================

function handleStop() {
    log('STOP');

    // Aktives Audio stoppen
    const activeAudio = mediaCache.audio[state.currentId];
    if (activeAudio) {
        activeAudio.pause();
        activeAudio.currentTime = 0;
    }

    // Auch das Haupt-Audio-Element stoppen (Fallback)
    const audio = elements.audio;
    if (audio) {
        audio.pause();
        audio.currentTime = 0;
    }

    state.isPlaying = false;
    elements.progressFill.style.width = '0%';
}

function handlePlay(id) {
    const startTime = performance.now();
    log(`PLAY: ${id}`);

    if (!state.audioUnlocked) {
        log('Audio nicht entsperrt!', 'warn');
        return;
    }

    // Vorheriges Audio stoppen
    if (state.currentId !== null && state.currentId !== id) {
        const prevAudio = mediaCache.audio[state.currentId];
        if (prevAudio) {
            prevAudio.pause();
            prevAudio.currentTime = 0;
        }
    }

    state.currentId = id;
    state.isPlaying = true;

    const id3 = id.toString().padStart(3, '0');

    // UI aktualisieren
    elements.currentId.textContent = id3;
    elements.mediaContainer.classList.add('active');
    elements.waiting.classList.add('hidden');

    // =========================================================================
    // OPTIMIERT: Gecachte Medien verwenden (instant!)
    // =========================================================================

    // Bild aus Cache oder neu laden
    const cachedImg = mediaCache.images[id];
    if (cachedImg) {
        elements.imageContainer.innerHTML = '';
        const imgClone = cachedImg.cloneNode();
        imgClone.alt = id3;
        elements.imageContainer.appendChild(imgClone);
        log(`Bild aus Cache: ${id3} (instant)`);
    } else {
        // Fallback: Neu laden
        const imageUrl = `/media/${id3}.jpg`;
        elements.imageContainer.innerHTML = `<img src="${imageUrl}" alt="${id3}" onerror="this.parentElement.innerHTML='<span class=\\'placeholder\\'>Bild nicht gefunden</span>'">`;
        log(`Bild neu geladen: ${id3}`);
    }

    // Audio aus Cache oder neu laden
    const cachedAudio = mediaCache.audio[id];
    if (cachedAudio) {
        // Event-Handler für gecachtes Audio setzen
        cachedAudio.onended = handleAudioEnded;
        cachedAudio.ontimeupdate = () => updateProgressFromAudio(cachedAudio);
        cachedAudio.onerror = (e) => {
            log(`Audio-Fehler: ${e.target.error?.message || 'unbekannt'}`, 'error');
        };

        cachedAudio.currentTime = 0;
        cachedAudio.play()
            .then(() => {
                const latency = Math.round(performance.now() - startTime);
                log(`Audio aus Cache gestartet: ${id3} (${latency}ms)`);
            })
            .catch((e) => log(`Audio-Fehler: ${e.message}`, 'error'));
    } else {
        // Fallback: Haupt-Audio-Element verwenden
        const audio = elements.audio;
        audio.src = `/media/${id3}.mp3`;
        audio.load();

        audio.play()
            .then(() => {
                const latency = Math.round(performance.now() - startTime);
                log(`Audio neu geladen: ${id3} (${latency}ms)`);
            })
            .catch((e) => log(`Audio-Fehler: ${e.message}`, 'error'));
    }
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
// AUDIO-UNLOCK (Browser Autoplay Policy - iOS-kompatibel)
// =============================================================================

async function unlockAudio() {
    log('Audio-Unlock...');

    // Methode 1: AudioContext (funktioniert besser auf iOS)
    try {
        const AudioContext = window.AudioContext || window.webkitAudioContext;
        if (AudioContext) {
            const ctx = new AudioContext();

            // Kurzen stillen Ton erzeugen
            const buffer = ctx.createBuffer(1, 1, 22050);
            const source = ctx.createBufferSource();
            source.buffer = buffer;
            source.connect(ctx.destination);
            source.start(0);

            // AudioContext Resume (wichtig für iOS)
            if (ctx.state === 'suspended') {
                await ctx.resume();
            }

            log('AudioContext initialisiert');
        }
    } catch (e) {
        log(`AudioContext-Fehler: ${e.message}`, 'warn');
    }

    // Methode 2: HTML Audio Element
    const audio = elements.audio;
    audio.volume = 1;

    // Längeres stilles Audio (besser für iOS)
    const silentWav = 'data:audio/wav;base64,UklGRigAAABXQVZFZm10IBIAAAABAAEARKwAAIhYAQACABAAAABkYXRhAgAAAAEA';

    audio.src = silentWav;

    try {
        await audio.play();
        audio.pause();
        audio.currentTime = 0;
        audio.src = '';

        state.audioUnlocked = true;
        log('Audio entsperrt');

        elements.unlockBtn.classList.add('hidden');
        elements.waiting.classList.remove('hidden');
        elements.audioStatus.classList.remove('disconnected');
        elements.audioStatus.classList.add('connected');

        // =====================================================================
        // NEU v2.3.0: Medien vorladen nach Unlock
        // =====================================================================
        await preloadAllMedia();

    } catch (e) {
        log(`Unlock fehlgeschlagen: ${e.message}`, 'error');

        // Fallback: Nur AudioContext war erfolgreich
        if (window.AudioContext || window.webkitAudioContext) {
            state.audioUnlocked = true;
            log('Fallback: AudioContext OK');

            elements.unlockBtn.classList.add('hidden');
            elements.waiting.classList.remove('hidden');
            elements.audioStatus.classList.remove('disconnected');
            elements.audioStatus.classList.add('connected');

            // Trotzdem preloaden versuchen
            await preloadAllMedia();
        } else {
            elements.audioStatus.classList.add('disconnected');
        }
    }
}

// =============================================================================
// FORTSCHRITTSANZEIGE
// =============================================================================

function updateProgress() {
    const audio = elements.audio;
    updateProgressFromAudio(audio);
}

function updateProgressFromAudio(audio) {
    if (!audio || !audio.duration || !isFinite(audio.duration)) return;

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

    // Audio-Events (Haupt-Element, Fallback)
    elements.audio.addEventListener('ended', handleAudioEnded);
    elements.audio.addEventListener('timeupdate', updateProgress);
    elements.audio.addEventListener('error', (e) => {
        log(`Audio-Fehler: ${e.target.error?.message || 'unbekannt'}`, 'error');
    });

    // Debug-Toggle
    if (elements.debugToggle) {
        elements.debugToggle.addEventListener('click', () => {
            const isVisible = elements.debug.classList.toggle('visible');
            elements.debugToggle.textContent = isVisible ? 'Debug [X]' : 'Debug';
            elements.debugToggle.classList.toggle('active', isVisible);
        });
    }

    // Keyboard-Shortcuts
    document.addEventListener('keydown', (e) => {
        // Space: Play/Pause
        if (e.code === 'Space' && state.audioUnlocked) {
            e.preventDefault();
            const activeAudio = mediaCache.audio[state.currentId] || elements.audio;
            if (activeAudio) {
                activeAudio.paused ? activeAudio.play() : activeAudio.pause();
            }
        }

        // Ctrl+D: Debug-Panel
        if (e.code === 'KeyD' && e.ctrlKey) {
            e.preventDefault();
            if (elements.debug && elements.debugToggle) {
                const isVisible = elements.debug.classList.toggle('visible');
                elements.debugToggle.textContent = isVisible ? 'Debug [X]' : 'Debug';
                elements.debugToggle.classList.toggle('active', isVisible);
            }
        }
    });
}

// =============================================================================
// INITIALISIERUNG
// =============================================================================

function init() {
    log('Dashboard v2.3.0 wird initialisiert...');
    log(`Konfiguration: ${CONFIG.numMedia} Medien, Preload-Concurrency: ${CONFIG.preloadConcurrency}`);

    setupEventListeners();

    elements.wsStatus.classList.add('disconnected');
    elements.audioStatus.classList.add('disconnected');

    connectWebSocket();

    log('Dashboard bereit - klicke "Sound aktivieren" zum Starten');
}

document.addEventListener('DOMContentLoaded', init);
