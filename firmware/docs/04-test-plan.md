# Testplan

## Phase A: LED-Test (74HC595, HW-SPI)

**Ziel:** Verifizieren dass alle LEDs korrekt angesteuert werden.

**DoD (Definition of Done):**
- [ ] Lauflicht LED 1 → LED 10 korrekt (keine Lücken, keine Doppel)
- [ ] Log zeigt Bytes passend zum Mapping (LED 1 = Bit 0)
- [ ] PWM-Helligkeit funktioniert (OE active-low)
- [ ] `LEDCLR` schaltet alle LEDs aus
- [ ] `LEDALL` schaltet alle LEDs an

**Testbefehle:**
```
LEDSET 001
LEDSET 005
LEDSET 010
LEDCLR
LEDALL
```

---

## Phase B: Button-Test (CD4021B, HW-SPI)

**Ziel:** Verifizieren dass alle Taster korrekt erkannt werden.

**DoD:**
- [ ] BTN 1 → `PRESS 001`, BTN 10 → `PRESS 010`
- [ ] Kein "BTN 1 fehlt" (First-Bit Rescue bestätigt)
- [ ] RAW/DEB plausibel (DEB stabil nach 30 ms)
- [ ] RELEASE-Events kommen nach Loslassen
- [ ] Keine Phantom-Presses bei unbenutzten Eingängen

**Testprozedur:**
1. Jeden Taster einzeln drücken
2. Taster gedrückt halten (> 1s)
3. Taster loslassen
4. Schnelles Mehrfachdrücken (Debounce-Test)

---

## Phase C: Integration (Buttons → LEDs, One-Hot)

**Ziel:** Verifizieren dass Taster korrekt mit LEDs verknüpft sind.

**DoD:**
- [ ] Taster n setzt LED n (1:1 Mapping)
- [ ] One-Hot: nie mehr als eine LED aktiv
- [ ] Auswahl-Policy funktioniert (LATCH_SELECTION)
- [ ] LED bleibt an nach Loslassen (wenn LATCH=true)

**Testmatrix:**

| Aktion | Erwartung (LATCH=true) |
|--------|------------------------|
| BTN 3 drücken | LED 3 an, andere aus |
| BTN 3 loslassen | LED 3 bleibt an |
| BTN 7 drücken | LED 7 an, LED 3 aus |
| LEDCLR senden | Alle LEDs aus |

---

## Phase D: FreeRTOS Stabilität

**Ziel:** Verifizieren dass das System unter Last stabil läuft.

**DoD:**
- [ ] IO-Periodik stabil (5 ms ± 0.5 ms)
- [ ] Serial-Task kann lang loggen ohne IO-Jitter
- [ ] Keine Reboots/Watchdog/Stack Overflows
- [ ] 24h Dauertest bestanden

**Metriken:**
```cpp
// In io_task.cpp
static uint32_t maxJitter = 0;
uint32_t jitter = abs(now - expectedTime);
if (jitter > maxJitter) maxJitter = jitter;
```

---

## Phase E: Pi-Integration

**Ziel:** Verifizieren dass ESP32 und Pi korrekt kommunizieren.

**DoD:**
- [ ] `READY` und `FW` beim ESP32-Start empfangen
- [ ] `PING` → `PONG` funktioniert
- [ ] `PRESS` Events kommen im server.py an
- [ ] `LEDSET` von server.py schaltet korrekte LED
- [ ] WebSocket-Events im Browser sichtbar
- [ ] Medien-Wiedergabe funktioniert

**Testbefehle (vom Pi):**
```bash
echo "PING" > /dev/serial/by-id/usb-Espressif*
echo "LEDSET 005" > /dev/serial/by-id/usb-Espressif*
echo "STATUS" > /dev/serial/by-id/usb-Espressif*
```
