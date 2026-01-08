# Selection Panel – Hardware Test Firmware (ESP32-S3)

Firmware-Sammlung zum Verifizieren der Hardware-Kette für ein Selection-Panel:

- **CD4021B** (Taster-Eingänge, active-low)
- **74HC595** (LED-Ausgänge, active-high)
- gemeinsamer **SPI-SCK**, getrennte **MISO/MOSI**, plus **LOAD/LATCH/OE**

Ziel: stabile, reproduzierbare Tests als Basis für die modulare Firmware + spätere Pi-Integration.

---

## Projektstruktur

```text
hardwaretest_firmware/
├── HARDWARE.md
├── phase1_esp32_test/           ESP32 Grundtests (Arduino vs. FreeRTOS / Timing)
├── phase2_led_spi_test/         74HC595 LED-Test via HW-SPI (Lauflicht / Mapping)
├── phase3_button_spi_test/      CD4021B Button-Test via HW-SPI (RAW / Mapping)
├── phase4_combined_spi_test/    Kombi: Taster → LED (End-to-End)
├── phase5_FreeRTOS/             FreeRTOS Task-Split (IO vs Serial), Queue/Timing
├── phase6_modular/              Modulare Architektur (hal/drivers/logic/app)
├── phase7_pi/                   = firmware/ Pi-Anbindung (Platzhalter/Integration)
└── README.md
```

---

## Hardware (Kurz)

Details siehe **`HARDWARE.md`**.

### Pinbelegung (XIAO ESP32-S3)

| Pin | Funktion             | Baustein          |
| --: | -------------------- | ----------------- |
|  D8 | SCK (Clock)          | CD4021B + 74HC595 |
|  D1 | P/S (Load)           | CD4021B           |
|  D9 | MISO (Data In)       | CD4021B           |
| D10 | MOSI (Data Out)      | 74HC595           |
|  D0 | RCK (Latch)          | 74HC595           |
|  D2 | OE (PWM, active-low) | 74HC595           |

### Breadboard-Stabilität (wichtig)

Wenn CD4021 und 74HC595 den gleichen **SCK** teilen, taktet ein Button-Read den 74HC595 mit (es werden i. d. R. `0x00` geschoben). Ein **Latch-Glitch** kann dann LEDs „aus“ latchen.

Minimal (empfohlen):

- **RCK (Pin 12, gemeinsam für beide 74HC595)**: **1× 10 kΩ Pull-Down nach GND**
- **OE (Pin 13, active-low)**: **1× 10 kΩ Pull-Up nach 3,3 V** (Boot-sicher AUS)
- **/MR (Pin 10, active-low)**: **fest an 3,3 V** (oder Pull-Up)
- **100 nF** je IC an VCC/GND (hast du bereits)

Optional:

- **33–100 Ω** Serie in **RCK** (Dämpfung)

---

## Bit-Mapping (Merksatz)

- **CD4021B (Buttons): MSB-first**, active-low
  `Bit7 = T1 … Bit0 = T8` (weitere Bits in Byte 2)
- **74HC595 (LEDs): LSB-first**, active-high
  `Bit0 = LED1, Bit1 = LED2, …`

---

## Build & Flash (PlatformIO)

Voraussetzungen:

- VS Code + PlatformIO
- Board: Seeed XIAO ESP32-S3 (USB-CDC)

Typischer Ablauf pro Phase:

```bash
cd hardwaretest_firmware/<phaseX_...>
pio run -t upload
pio device monitor
```

Falls mehrere Environments existieren:

```bash
pio run -e <env> -t upload
pio device monitor -e <env>
```

---

## Phasen – was du jeweils testest

1. **phase1_esp32_test**
   Grundsetup, Timing, ggf. Arduino vs FreeRTOS Vergleich.

2. **phase2_led_spi_test**
   74HC595 Mapping + Latch/OE/PWM (Lauflicht).
   Ziel: LED-Ausgänge sicher und reproduzierbar.

3. **phase3_button_spi_test**
   CD4021B RAW-Read via SPI, active-low Interpretation, Mapping.
   Ziel: Taster 1..10 korrekt erkannt.

4. **phase4_combined_spi_test**
   End-to-End: Taster n → LED n (inkl. Mapping-Brücke MSB↔LSB).

5. **phase5_FreeRTOS**
   IO-Task periodisch, Serial-Task event-basiert (Queue), keine Serial-Prints im IO-Pfad.

6. **phase6_modular**
   Zielarchitektur (hal/drivers/logic/app). Skalierbar 10→100.

7. **phase7_pi**
   Vorbereitung / Integration ESP32 ↔ Raspberry Pi (Serial-Bridge / Protokoll).

---

## Skalierung auf 100 Taster/LEDs

In der jeweiligen Firmware-Konfiguration (typisch `include/config.h`) die Zähler erhöhen:

```cpp
constexpr uint8_t BTN_COUNT = 100;
constexpr uint8_t LED_COUNT = 100;
```

Die Byte-Längen werden i. d. R. automatisch abgeleitet.

---

## Status

- [x] Phase 1: ESP32 Grundtests
- [x] Phase 2: 74HC595 LED-SPI Test
- [x] Phase 3: CD4021 Button-SPI Test
- [x] Phase 4: Kombi (Button → LED)
- [x] Phase 5: FreeRTOS-Variante
- [x] Phase 6: Modulare Firmware (Basis)
- [x] Phase 7: Pi-Integration (Ausbau)
