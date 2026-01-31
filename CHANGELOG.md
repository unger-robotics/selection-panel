# Changelog

Alle relevanten Aenderungen am Projekt werden in dieser Datei dokumentiert.

Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/).
Versionierung folgt [Semantic Versioning](https://semver.org/lang/de/).

---



## Versionsschema

```
MAJOR.MINOR.PATCH

MAJOR - Inkompatible API-Aenderungen
MINOR - Neue Features (rueckwaertskompatibel)
PATCH - Bugfixes (rueckwaertskompatibel)
```

## Aenderungstypen

- **Hinzugefuegt** - Neue Features
- **Geaendert** - Aenderungen an bestehenden Features
- **Veraltet** - Features die bald entfernt werden
- **Entfernt** - Entfernte Features
- **Behoben** - Bugfixes
- **Sicherheit** - Sicherheitsrelevante Aenderungen

---

## [2.5.3] - 2026-01-31

### Geaendert

- **Projektstruktur**: Reorganisiert fuer bessere Trennung
  - `server/` - Python Backend + Medien
  - `dashboard/` - Web Frontend (index.html + static/)
  - `firmware/` - ESP32 Firmware (unveraendert)
- Server-Pfade angepasst fuer neue Struktur
- Dokumentation aktualisiert (CLAUDE.md, README.md)
