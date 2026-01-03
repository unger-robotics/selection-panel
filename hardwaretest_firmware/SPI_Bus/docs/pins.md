# Pins

## Pinbelegung (Prototyp 10× Taster + 10× LEDs, später 100× optional)

**Ziel:** gemeinsamer **Hardware-SPI-Bus** für **74HC595 (LED-Out)** + **CD4021 (Taster-In)**

* **SCK** gemeinsam (Takt)
* **MOSI** nur **zum 74HC595** (Daten raus)
* **MISO** nur **vom CD4021** (Daten rein)
* extra GPIO: **LATCH_595** und **LOAD_4021**

---

## A) ESP32-S3 XIAO → Signale (SPI + Steuerleitungen)

**Hardware-SPI beim XIAO ESP32-S3:**

* **SCK = D8 / GPIO7**
* **MISO = D9 / GPIO8**
* **MOSI = D10 / GPIO9** ([Seeed Studio Wiki][1])

**Deine Wahl:**

* **LATCH_595 = D0 (GPIO1)**
* **LOAD_4021 = D1 (GPIO2)** ([PlatformIO Community][2])

| Funktion  | XIAO Pin | GPIO | Richtung | Geht zu                                            |
| --------- | -------: | ---: | -------- | -------------------------------------------------- |
| SPI SCK   |       D8 |    7 | Out      | 74HC595 SRCLK (Pin 11) **und** CD4021 CLK (Pin 10) |
| SPI MOSI  |      D10 |    9 | Out      | 74HC595 SER (Pin 14)                               |
| SPI MISO  |       D9 |    8 | In       | CD4021 Q8 (Pin 3) von **IC#0 (am ESP)**            |
| LATCH_595 |       D0 |    1 | Out      | 74HC595 RCLK (Pin 12) Bus                          |
| LOAD_4021 |       D1 |    2 | Out      | CD4021 P/S (Pin 9) Bus                             |
| GND       |      GND |    – | –        | alle IC-GND + Taster-GND                           |
| 3.3 V     |      3V3 |    – | –        | alle IC-VCC/VDD + Pull-ups                         |

---

## B) LED-Kette (74HC595) – Prototyp 10 LEDs = 2 ICs

### Bus-Verdrahtung (alle 595 gleich)

| 74HC595 Pin | Name  | Verbindung                          |
| ----------: | ----- | ----------------------------------- |
|          16 | VCC   | 3.3 V                               |
|           8 | GND   | GND                                 |
|          11 | SRCLK | SPI SCK (D8/GPIO7)                  |
|          12 | RCLK  | LATCH_595 (D0/GPIO1)                |
|          14 | SER   | SPI MOSI (D10/GPIO9) → **nur IC#0** |
|           9 | QH′   | → SER (Pin 14) des nächsten IC      |
|          13 | OE    | GND (Outputs immer aktiv)           |
|          10 | SRCLR | 3.3 V (nicht löschen)               |

### IC-Reihenfolge und LED-Zuordnung

| IC | Position | LEDs                          | Bits      |
| -: | -------- | ----------------------------- | --------- |
|  0 | am ESP   | LED1…LED8                     | Bit0…Bit7 |
|  1 | dahinter | LED9…LED16 (nutzt LED9…LED10) | Bit0…Bit7 |

> **Hinweis zur Ausgangsbezeichnung:** Bit0 = **Q0/QA**, Bit1 = **Q1/QB**, …, Bit7 = **Q7/QH**.

---

## C) Taster-Kette (CD4021) – Prototyp 10 Taster = 2 ICs

### Bus-Verdrahtung (alle 4021 gleich)

| CD4021 Pin | Name | Verbindung                                                                        |
| ---------: | ---- | --------------------------------------------------------------------------------- |
|         16 | VDD  | 3.3 V                                                                             |
|          8 | VSS  | GND                                                                               |
|         10 | CLK  | SPI SCK (D8/GPIO7)                                                                |
|          9 | P/S  | LOAD_4021 (D1/GPIO2)                                                              |
|          3 | Q8   | **IC#0 → SPI MISO** (D9/GPIO8)                                                    |
|         11 | SER  | von Q8 des **nächsten** IC; letzter IC: **fest auf 3.3 V oder GND** (nicht offen) |

### Warum deine alte Zuordnung nicht passte (P1=BTN1 … P8=BTN8)

Beim CD4021 liegt nach dem Load-Umschalten **PI-8 (P8)** *als erstes* am seriellen Ausgang **Q8** an, danach PI-7, … bis PI-1. ([Arduino][3])
Wenn du also „P1=BTN1 … P8=BTN8“ verdrahtest, kommt **BTN8 zuerst** raus → deine Bits wirken „verdreht“.

### Verdrahtung pro IC für lineare Button-Nummerierung (ohne Lookup)

Damit gilt später sauber: `ic=(n-1)/8`, `bit=(n-1)%8` und **BTN1 landet auf Bit0**.

**Pro IC #k:**

* BTN(8k+1) → **P8**
* BTN(8k+2) → **P7**
* BTN(8k+3) → **P6**
* BTN(8k+4) → **P5**
* BTN(8k+5) → **P4**
* BTN(8k+6) → **P3**
* BTN(8k+7) → **P2**
* BTN(8k+8) → **P1**

**Pins (CD4021 DIP-16):**

* P8=Pin15, P7=Pin7, P6=Pin6, P5=Pin5, P4=Pin4, P3=Pin14, P2=Pin13, P1=Pin1

#### Prototyp-Tabelle BTN1…BTN10

| BTN | IC | Parallel-Input | CD4021 Pin |
| --: | -: | -------------- | ---------: |
|   1 |  0 | P8             |         15 |
|   2 |  0 | P7             |          7 |
|   3 |  0 | P6             |          6 |
|   4 |  0 | P5             |          5 |
|   5 |  0 | P4             |          4 |
|   6 |  0 | P3             |         14 |
|   7 |  0 | P2             |         13 |
|   8 |  0 | P1             |          1 |
|   9 |  1 | P8             |         15 |
|  10 |  1 | P7             |          7 |

### Pull-ups (pro Taster)

* jeweiliger P-Pin → **10 kΩ nach 3.3 V**
* Taster schaltet P-Pin nach **GND**
* Ergebnis: **gedrückt = 0 (active-low)**

---

## Erweiterung auf 100× (optional)

### Anzahl ICs

* 100 LEDs → **13× 74HC595** (104 Kanäle)
* 100 Taster → **13× CD4021** (104 Kanäle)

### Ketten-Regel

* **IC #0** sitzt jeweils **am ESP**

  * 595: IC#0 bekommt MOSI direkt
  * 4021: IC#0 liefert Q8 direkt nach MISO
* Kaskade:

  * 595: `QH′ → SER` zum nächsten IC
  * 4021: `Q8 (weiter hinten) → SER (weiter vorne Richtung ESP)`

### Zuordnung (Formel)

* LED(n): `ic=(n-1)/8`, `bit=(n-1)%8`
* BTN(n): `ic=(n-1)/8`, `bit=(n-1)%8` (weil du P8→…→P1 so verdrahtest)

[1]: https://wiki.seeedstudio.com/xiao_esp32s3_pin_multiplexing/ "Pin Multiplexing with Seeed Studio XIAO ESP32S3 (Sense) | Seeed Studio Wiki"
[2]: https://community.platformio.org/t/seeed-xiao-esp32s3-pin-mapping/47599?utm_source=chatgpt.com "Seeed_xiao_esp32s3 pin mapping"
[3]: https://www.arduino.cc/en/Tutorial/ShiftIn?utm_source=chatgpt.com "CD4021B Shift Registers | Arduino Documentation"
