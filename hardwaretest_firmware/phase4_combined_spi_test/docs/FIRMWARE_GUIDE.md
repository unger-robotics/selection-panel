# Embedded Firmware mit Arduino C++

Selection-Panel-Codes

---

## 1. Was ist Embedded Firmware?

**Firmware** ist Software, die direkt auf Hardware läuft – ohne Betriebssystem dazwischen. Sie steuert Mikrocontroller, die in Geräten "eingebettet" (embedded) sind.

```
┌─────────────────────────────────────────────────────────┐
│                    Desktop-PC                           │
├─────────────────────────────────────────────────────────┤
│  Anwendung (Browser, Word, ...)                         │
│  ─────────────────────────────────────────────────────  │
│  Betriebssystem (Windows, Linux, macOS)                 │
│  ─────────────────────────────────────────────────────  │
│  Hardware (CPU, RAM, SSD, ...)                          │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│                  Embedded System                        │
├─────────────────────────────────────────────────────────┤
│  Firmware (dein Code)           ← Läuft direkt!         │
│  ─────────────────────────────────────────────────────  │
│  Hardware (ESP32, Sensoren, LEDs, ...)                  │
└─────────────────────────────────────────────────────────┘
```

**Konsequenzen für den Code:**

| Aspekt | Desktop | Embedded |
|--------|---------|----------|
| Speicher | GBs RAM | KBs RAM |
| Dateisystem | Ja | Meist nein |
| Multitasking | OS regelt | Du regelst |
| Timing | Unkritisch | Oft kritisch |
| Fehler | Absturz → Neustart | Kann Hardware beschädigen |

---

## 2. Arduino C++: Welche Version?

### Der Arduino-Core

Arduino verwendet **C++11** mit Erweiterungen. Der ESP32-Arduino-Core (Espressif) basiert auf **GCC 8.4** und unterstützt:

- C++11 vollständig
- C++14 teilweise (z.B. keine Digit-Separatoren `1'000'000`)
- C++17 teilweise

### Was Arduino hinzufügt

Arduino ist kein eigener Compiler, sondern ein **Framework** auf C++:

```cpp
// Klassisches C/C++: Du schreibst main()
int main() {
    // Hardware initialisieren
    while (1) {
        // Endlosschleife
    }
    return 0;
}

// Arduino: Das Framework versteckt main()
void setup() {
    // Wird einmal ausgeführt
}

void loop() {
    // Wird endlos wiederholt
}
```

**Hinter den Kulissen** sieht Arduino's `main()` so aus:

```cpp
int main() {
    init();           // Hardware-Initialisierung
    setup();          // Dein Setup
    while (1) {
        loop();       // Dein Loop
    }
    return 0;
}
```

---

## 3. Programmierdogma: Embedded Best Practices

### Die goldenen Regeln

```
1. KEIN dynamischer Speicher (new, malloc) in der Hauptschleife
2. KEINE Blockierung (delay nur wenn unvermeidbar)
3. DETERMINISMUS: Jeder Durchlauf dauert gleich lang
4. FEHLERTOLERANZ: Hardware kann jederzeit "spinnen"
5. RESSOURCEN-BEWUSSTSEIN: Jedes Byte zählt
```

### Speicher-Dogma

```cpp
// ❌ SCHLECHT: Dynamische Allokation
void loop() {
    String text = "Hello";        // String alloziert auf Heap
    text += " World";             // Re-Allokation!
}
// → Speicherfragmentierung, irgendwann Absturz

// ✅ GUT: Statische Allokation
static char text[32];             // Feste Größe, einmal alloziert
void loop() {
    snprintf(text, sizeof(text), "Hello World");
}
```

### Timing-Dogma

```cpp
// ❌ SCHLECHT: Blockierendes Warten
void loop() {
    if (buttonPressed()) {
        doSomething();
        delay(1000);              // CPU macht 1 Sekunde NICHTS
    }
}

// ✅ GUT: Nicht-blockierendes Warten
static uint32_t lastAction = 0;
void loop() {
    if (buttonPressed() && (millis() - lastAction >= 1000)) {
        doSomething();
        lastAction = millis();
    }
}
```

---

## 4. Code-Aufbau: Die Anatomie der Firmware

### Dateistruktur

```
src/
├── config.h      ← Konfiguration (Konstanten, Pins)
└── main.cpp      ← Hauptprogramm (Logik)
```

### Aufbau von main.cpp

```cpp
// ┌─────────────────────────────────────────────────────┐
// │ 1. INCLUDES                                         │
// │    Externe Bibliotheken einbinden                   │
// └─────────────────────────────────────────────────────┘
#include "config.h"
#include <SPI.h>

// ┌─────────────────────────────────────────────────────┐
// │ 2. KONSTANTEN & KONFIGURATION                       │
// │    SPI-Settings, Timing-Parameter                   │
// └─────────────────────────────────────────────────────┘
static const SPISettings spiButtons(500000, MSBFIRST, SPI_MODE1);

// ┌─────────────────────────────────────────────────────┐
// │ 3. GLOBALE VARIABLEN (ZUSTAND)                      │
// │    static = nur in dieser Datei sichtbar            │
// └─────────────────────────────────────────────────────┘
static uint8_t btnRaw[BTN_BYTES];
static uint8_t activeId = 0;

// ┌─────────────────────────────────────────────────────┐
// │ 4. HILFSFUNKTIONEN                                  │
// │    Kleine, wiederverwendbare Bausteine              │
// └─────────────────────────────────────────────────────┘
static inline bool btnIsPressed(const uint8_t *state, uint8_t id) {
    // ...
}

// ┌─────────────────────────────────────────────────────┐
// │ 5. HARDWARE-FUNKTIONEN                              │
// │    Direkter Zugriff auf Peripherie                  │
// └─────────────────────────────────────────────────────┘
static void readButtons() {
    // ...
}

// ┌─────────────────────────────────────────────────────┐
// │ 6. LOGIK-FUNKTIONEN                                 │
// │    Verarbeitung, Entscheidungen                     │
// └─────────────────────────────────────────────────────┘
static void debounceButtons() {
    // ...
}

// ┌─────────────────────────────────────────────────────┐
// │ 7. DEBUG-FUNKTIONEN                                 │
// │    Ausgaben für Entwicklung                         │
// └─────────────────────────────────────────────────────┘
static void printState() {
    // ...
}

// ┌─────────────────────────────────────────────────────┐
// │ 8. ARDUINO ENTRY POINTS                             │
// │    setup() und loop()                               │
// └─────────────────────────────────────────────────────┘
void setup() {
    // Einmalige Initialisierung
}

void loop() {
    // Hauptschleife
}
```

---

## 5. C++ Syntax im Detail

### 5.1 Präprozessor-Direktiven

Der Präprozessor läuft **vor** dem Compiler und manipuliert den Quelltext:

```cpp
// Include: Fügt Dateiinhalt ein
#include <SPI.h>          // Sucht in System-Pfaden
#include "config.h"       // Sucht im Projekt-Ordner

// Include Guard: Verhindert doppeltes Einbinden
#pragma once              // Moderne Variante (eine Zeile)

// Alternativ (klassisch):
#ifndef CONFIG_H
#define CONFIG_H
// ... Inhalt ...
#endif
```

### 5.2 Datentypen

#### Ganzzahlen (Integer)

```cpp
// Arduino/C++ Standard-Typen (Größe variiert je nach Plattform!)
int x;            // Mindestens 16 Bit, auf ESP32: 32 Bit
long y;           // Mindestens 32 Bit
char c;           // 8 Bit (oft für Zeichen)

// Embedded Best Practice: Exakte Größen verwenden!
#include <cstdint>        // Oder: <stdint.h>

int8_t   a;       //  8 Bit mit Vorzeichen:    -128 bis 127
uint8_t  b;       //  8 Bit ohne Vorzeichen:      0 bis 255
int16_t  c;       // 16 Bit mit Vorzeichen: -32768 bis 32767
uint16_t d;       // 16 Bit ohne Vorzeichen:      0 bis 65535
int32_t  e;       // 32 Bit mit Vorzeichen
uint32_t f;       // 32 Bit ohne Vorzeichen

// size_t: Für Größen und Indizes (plattformabhängig, immer positiv)
size_t arraySize = 10;
```

#### Warum exakte Größen?

```cpp
// ❌ Problem: "int" ist auf verschiedenen Plattformen unterschiedlich groß
int value = 70000;        // Auf 16-Bit-System: Überlauf!

// ✅ Lösung: Explizite Größe
uint32_t value = 70000;   // Immer 32 Bit, egal welche Plattform
```

#### Konstanten

```cpp
// C-Style (veraltet, aber funktioniert)
#define LED_COUNT 10      // Textersetzung, kein Typ!

// C++ Style (empfohlen)
const int LED_COUNT = 10;           // Zur Laufzeit, braucht RAM
constexpr int LED_COUNT = 10;       // Zur Compile-Zeit, kein RAM!
```

**constexpr vs const:**

```cpp
constexpr int COMPILE_TIME = 5 * 2;     // Berechnung zur Compile-Zeit
const int RUNTIME = analogRead(A0);      // Kann nur const sein (Laufzeit)
```

### 5.3 Variablen-Deklaration

```cpp
// Speicherklassen
int globalVar;                    // Global: überall sichtbar
static int fileVar;               // Datei-lokal: nur in dieser .cpp
void func() {
    int localVar;                 // Lokal: nur in dieser Funktion
    static int persistentVar;     // Lokal, aber behält Wert zwischen Aufrufen!
}
```

**static bei globalen Variablen:**

```cpp
// In main.cpp:
static uint8_t btnRaw[BTN_BYTES];

// "static" hier bedeutet: Diese Variable ist nur in main.cpp sichtbar.
// Andere .cpp-Dateien können keine Variable mit demselben Namen sehen.
// → Vermeidet Namenskonflikte in größeren Projekten.
```

**static bei lokalen Variablen:**

```cpp
void countCalls() {
    static int counter = 0;   // Initialisierung nur beim ersten Aufruf!
    counter++;
    Serial.println(counter);
}

// Aufruf 1: Ausgabe "1"
// Aufruf 2: Ausgabe "2"
// Aufruf 3: Ausgabe "3"
// → Variable "überlebt" zwischen Funktionsaufrufen
```

### 5.4 Funktionen

#### Grundstruktur

```cpp
Rückgabetyp Funktionsname(Parameter) {
    // Körper
    return Wert;    // Falls Rückgabetyp nicht void
}
```

#### Beispiele aus dem Code

```cpp
// Keine Parameter, keine Rückgabe
void setup() {
    // ...
}

// Parameter und Rückgabe
static inline bool btnIsPressed(const uint8_t *state, uint8_t id) {
    // state: Zeiger auf Array (wird nicht verändert wegen const)
    // id: Kopie des Wertes (call by value)
    return !(state[byte] & (1u << bit));
}

// Nur Parameter, keine Rückgabe
static void ledSet(uint8_t id, bool on) {
    if (id < 1 || id > LED_COUNT) return;   // Frühes Return bei Fehler
    // ...
}
```

#### Schlüsselwörter bei Funktionen

```cpp
// static: Funktion nur in dieser Datei sichtbar
static void readButtons() { }

// inline: Compiler soll Code direkt einsetzen statt Funktionsaufruf
//         (Hinweis, keine Garantie)
inline bool isValid(int x) { return x > 0; }

// static inline: Beides kombiniert (häufig für kleine Hilfsfunktionen)
static inline uint8_t byteIndex(uint8_t id) { return (id - 1) / 8; }
```

### 5.5 Zeiger und Referenzen

#### Zeiger (Pointer)

```cpp
uint8_t buffer[10];           // Array
uint8_t *ptr = buffer;        // Zeiger auf erstes Element

*ptr = 42;                    // Dereferenzierung: Wert schreiben
uint8_t x = *ptr;             // Dereferenzierung: Wert lesen
ptr++;                        // Zeiger auf nächstes Element

// const-Korrektheit
const uint8_t *readOnly = buffer;    // Daten nicht änderbar
*readOnly = 5;                        // ❌ Compiler-Fehler!
```

#### Arrays und Zeiger

```cpp
// In C/C++ sind Arrays und Zeiger eng verwandt:
uint8_t arr[10];
uint8_t *ptr = arr;           // Array "zerfällt" zu Zeiger

arr[3] = 5;                   // Identisch:
*(arr + 3) = 5;               // Zeiger-Arithmetik
ptr[3] = 5;                   // Auch Zeiger kann [] nutzen
```

#### Funktionsparameter

```cpp
// Call by Value: Kopie wird übergeben
void modify(int x) {
    x = 99;                   // Ändert nur die lokale Kopie
}

// Call by Pointer: Adresse wird übergeben
void modify(int *x) {
    *x = 99;                  // Ändert das Original
}

// Call by Reference (C++): Wie Pointer, aber eleganter
void modify(int &x) {
    x = 99;                   // Ändert das Original
}

// Const Reference: Effizient + nicht änderbar
void print(const String &s) {
    Serial.println(s);        // Keine Kopie, kein Ändern
}
```

### 5.6 Bit-Operationen

Essentiell für Embedded-Entwicklung!

```cpp
// Bit-Operatoren
uint8_t a = 0b11001010;
uint8_t b = 0b10101100;

a & b     // AND:  0b10001000  (beide 1 → 1)
a | b     // OR:   0b11101110  (mindestens einer 1 → 1)
a ^ b     // XOR:  0b01100110  (genau einer 1 → 1)
~a        // NOT:  0b00110101  (invertiert)
a << 2    // Links-Shift: 0b00101000 (× 4)
a >> 2    // Rechts-Shift: 0b00110010 (÷ 4)

// Praktische Anwendungen:
// Bit setzen (auf 1)
value |= (1u << bitNr);       // 1u = unsigned, verhindert Vorzeichen-Probleme

// Bit löschen (auf 0)
value &= ~(1u << bitNr);

// Bit umschalten (toggle)
value ^= (1u << bitNr);

// Bit prüfen
if (value & (1u << bitNr)) {
    // Bit ist gesetzt
}
```

#### Im Code

```cpp
// Taster gedrückt prüfen (Active-Low: 0 = gedrückt)
return !(state[byte] & (1u << bit));
//       ↑              ↑
//       Invertieren    Bit-Maske

// LED einschalten
ledState[byte] |= (1u << bit);

// LED ausschalten
ledState[byte] &= ~(1u << bit);
```

### 5.7 Kontrollstrukturen

```cpp
// if-else
if (bedingung) {
    // ...
} else if (andere_bedingung) {
    // ...
} else {
    // ...
}

// Kurzform für einfache Zuweisungen
int max = (a > b) ? a : b;    // Ternärer Operator

// for-Schleife
for (int i = 0; i < 10; i++) {
    // i läuft von 0 bis 9
}

// Rückwärts (wichtig für Daisy-Chain!)
for (int i = LED_BYTES - 1; i >= 0; --i) {
    SPI.transfer(ledState[i]);
}

// while-Schleife
while (bedingung) {
    // Solange bedingung wahr
}

// do-while (mindestens einmal ausführen)
do {
    // ...
} while (bedingung);

// Frühes Verlassen
for (int i = 0; i < 100; i++) {
    if (found) break;         // Schleife sofort verlassen
    if (skip) continue;       // Zum nächsten Durchlauf springen
}
```

### 5.8 Speicher-Funktionen

```cpp
#include <cstring>            // Oder: <string.h>

uint8_t src[10] = {1, 2, 3};
uint8_t dst[10];

// Kopieren
memcpy(dst, src, 10);         // 10 Bytes von src nach dst

// Füllen
memset(dst, 0x00, 10);        // 10 Bytes mit 0x00 füllen
memset(dst, 0xFF, 10);        // 10 Bytes mit 0xFF füllen

// Vergleichen
if (memcmp(src, dst, 10) == 0) {
    // Identisch
}
```

---

## 6. Arduino-spezifische Funktionen

### Digitale I/O

```cpp
pinMode(PIN, MODE);           // MODE: INPUT, OUTPUT, INPUT_PULLUP
digitalWrite(PIN, HIGH);      // HIGH oder LOW
int state = digitalRead(PIN); // Gibt HIGH oder LOW zurück
```

### Timing

```cpp
delay(1000);                  // 1000 ms warten (BLOCKIEREND!)
delayMicroseconds(100);       // 100 µs warten (BLOCKIEREND!)

uint32_t ms = millis();       // Millisekunden seit Start
uint32_t us = micros();       // Mikrosekunden seit Start
```

### Serielle Kommunikation

```cpp
Serial.begin(115200);         // Baudrate setzen
Serial.print("Text");         // Ohne Zeilenumbruch
Serial.println("Text");       // Mit Zeilenumbruch
Serial.printf("x=%d", x);     // Formatierte Ausgabe (ESP32)
```

### SPI

```cpp
#include <SPI.h>

SPI.begin(SCK, MISO, MOSI, SS);           // Pins konfigurieren
SPI.beginTransaction(SPISettings(...));    // Transaktion starten
uint8_t rx = SPI.transfer(tx);             // Byte senden/empfangen
SPI.endTransaction();                      // Transaktion beenden
```

### PWM (ESP32-spezifisch)

```cpp
ledcSetup(channel, freq, resolution);     // Kanal konfigurieren
ledcAttachPin(pin, channel);              // Pin zuweisen
ledcWrite(channel, duty);                 // Duty Cycle setzen
```

---

## 7. Der Code im Kontext

### Ablaufdiagramm

```
┌─────────────────────────────────────────────────────────────────┐
│                         setup()                                 │
├─────────────────────────────────────────────────────────────────┤
│  1. Serial.begin()         → Debugging vorbereiten              │
│  2. pinMode()              → Pins konfigurieren                 │
│  3. SPI.begin()            → SPI-Bus starten                    │
│  4. ledcSetup()            → PWM konfigurieren                  │
│  5. memset()               → Variablen initialisieren           │
│  6. writeLEDs()            → LEDs ausschalten                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         loop()                                  │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐                                                │
│  │ readButtons │ → Hardware → btnRaw[]                          │
│  └──────┬──────┘                                                │
│         ▼                                                       │
│  ┌────────────────┐                                             │
│  │debounceButtons │ → btnRaw[] → btnDebounced[]                 │
│  └──────┬─────────┘                                             │
│         ▼                                                       │
│  ┌───────────────┐                                              │
│  │ updateActiveId│ → btnDebounced[] → activeId                  │
│  └──────┬────────┘                                              │
│         ▼                                                       │
│  ┌───────────────┐                                              │
│  │ ledSet/Clear  │ → activeId → ledState[]                      │
│  └──────┬────────┘                                              │
│         ▼                                                       │
│  ┌───────────────┐                                              │
│  │  writeLEDs    │ → ledState[] → Hardware                      │
│  └──────┬────────┘                                              │
│         ▼                                                       │
│  ┌───────────────┐                                              │
│  │  printState   │ → Serial (nur bei Änderung)                  │
│  └──────┬────────┘                                              │
│         ▼                                                       │
│      delay(2)     → ~500 Hz Loop-Frequenz                       │
│         │                                                       │
└─────────┼───────────────────────────────────────────────────────┘
          │
          └──────────────► Zurück zum Anfang von loop()
```

### Datenfluss

```
Hardware          Firmware                    Hardware
─────────         ────────                    ────────

Taster ──────────► btnRaw[]
                      │
                      ▼
                  btnDebounced[] (zeitverzögert, stabil)
                      │
                      ▼
                  activeId (0-10)
                      │
                      ▼
                  ledState[]
                      │
                      ▼
                                ──────────────► LEDs
```

---

## 8. Zusammenfassung

### Checkliste für guten Embedded-Code

```
□ Exakte Datentypen verwenden (uint8_t, uint32_t, ...)
□ static für datei-lokale Variablen und Funktionen
□ constexpr für Compile-Zeit-Konstanten
□ Kein dynamischer Speicher in loop()
□ Blockierende Wartezeiten vermeiden
□ Bit-Operationen statt Division/Multiplikation wo möglich
□ Fehlerprüfung bei Parametern (Range-Checks)
□ Aussagekräftige Namen (nicht x, temp, data)
□ Kommentare erklären WARUM, nicht WAS
```

### Schnellreferenz: Schlüsselwörter

| Schlüsselwort | Bedeutung |
|---------------|-----------|
| `static` | Datei-lokal (global) oder persistent (lokal) |
| `const` | Wert nicht änderbar (zur Laufzeit) |
| `constexpr` | Wert zur Compile-Zeit berechnet |
| `inline` | Compiler-Hinweis: Code direkt einsetzen |
| `volatile` | Wert kann sich "von außen" ändern (Hardware, ISR) |
| `void` | Kein Rückgabewert / generischer Zeiger |
