# Bleifreies Löten mit SAC305

Handlöten von **ESP32-Breakout-Boards** und **XT60-Steckern** mit bleifreiem Lot.

---

## 1. Ausrüstung

### Lötzinn

| Produkt | Legierung | Durchmesser | Reichelt-Nr. | Preis |
|:--------|:----------|:-----------:|:-------------|------:|
| Felder ISO-Core "Clear" | SAC305 (Sn96,5Ag3Cu0,5) | 0,5 mm | LZ FE CSA 0,5 25 | 45,99 € |

| Parameter | Wert |
|:----------|:-----|
| Schmelzbereich | 217–219 °C |
| Flussmittelanteil | 3,5 % (No-Clean) |

> **0,5 mm für alles:** THT, SMD, XT60 – bei großen Kontakten 5–6× zuführen.

### Lötstationen

| Gerät | Leistung | Einsatz | Temperatur | Spitze |
|:------|:--------:|:--------|:----------:|:-------|
| Ersa i-CON PICO | 68 W | THT, SMD | 360 °C | 0102CDLF16 (1,6 mm Meißel) |
| Ersa 150 S | 150 W | XT60 | 400 °C | 0152KD (5,3 mm Meißel) |

### Zubehör

| Produkt | Funktion | Reichelt-Nr. | Preis |
|:--------|:---------|:-------------|------:|
| Stannol X32-10i | Flussmittelstift | STANNOL X32-10I | 6,99 € |
| ERSA Tip-Reactivator | Spitzen regenerieren | ERSA 0TR01 | ~13 € |

---

## 2. Temperatur

$$T_{\text{Spitze}} = T_{\text{Schmelz}} + 120\,°\text{C}$$

| Arbeitsgang | Temperatur |
|:------------|:----------:|
| SMD, Fein-Pitch | 350–360 °C |
| THT, Pin-Header | 360–380 °C |
| XT60, Litzen ≥ 1,5 mm² | 400 °C |

| Situation | Anpassung |
|:----------|:----------|
| Große Masseflächen | +20–30 °C |
| Empfindliche Bauteile | −20 °C |

---

## 3. Technik

### Lötstelle in 5 Schritten

```
1. Spitze verzinnen      → Wärmeübertragung verbessern
2. Spitze ansetzen       → An Pad UND Bauteilbein
3. Warten (1–2 s)        → Wärme übertragen
4. Lot zuführen          → Von der Gegenseite
5. Abziehen              → Erst Lot, dann Spitze
```

**Gesamtzeit:** 2–4 Sekunden

### Entlöten

| Methode | Anwendung |
|:--------|:----------|
| Entlötlitze + Flux | SMD, kleine Mengen |
| Entlötpumpe | THT, große Mengen |

---

## 4. ESP32 Breakout löten

- [ ] i-CON PICO auf **360 °C**
- [ ] Spitze 0102CDLF16 (1,6 mm Meißel)
- [ ] Platine fixieren

```
Spitze verzinnen → An Pad+Pin → 1–2 s → Lot zuführen → Abziehen
```

---

## 5. XT60 Stecker löten

- [ ] Ersa 150 S auf **400 °C** (5 Min vorheizen)
- [ ] Spitze 0152KD (5,3 mm Meißel)
- [ ] Litze abisolieren (8–10 mm), verdrillen

```
Hülse vorverzinnen → Litze vorverzinnen → Einführen → 
Spitze seitlich → Warten (3–5 s) → Nicht bewegen bis erstarrt
```

---

## 6. Spitzenpflege

| Wann | Aktion |
|:-----|:-------|
| Vor jedem Löten | Messingwolle → frisch verzinnen |
| Vor dem Ausschalten | **Niemals blank** – Zinn drauf |
| Oxidiert (Lot perlt ab) | Tip-Reactivator bei 250–300 °C |

---

## 7. Qualitätskontrolle

| Gut | Mangelhaft |
|:----|:-----------|
| Glänzend bis seidenmatt | Rissig, körnig |
| Konkaver Meniskus | Kugelig |
| Vollständige Benetzung | Lot perlt ab |

> **Hinweis:** SAC305 glänzt weniger als verbleites Lot. Seidenmatt ist normal.

---

## 8. Sicherheit

- Dämpfe nicht einatmen – Absaugung verwenden
- Lötkolben nie unbeaufsichtigt
- Bleihaltiges und bleifreies Lot **nie mischen**

---

## 9. Einkaufsliste (Reichelt)

| Artikel | Art.-Nr. | Preis |
|:--------|:---------|------:|
| Lötzinn bleifrei 0,5 mm, 250 g | LZ FE CSA 0,5 25 | 45,99 € |
| Flussmittelstift No-Clean | STANNOL X32-10I | 6,99 € |
| **Gesamt** | | **52,98 €** |

---

*Stand: Dezember 2025*
