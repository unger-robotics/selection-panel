Hier ist die aktualisierte Anleitung, angepasst auf die neue Ordnerstruktur und die rekursive Arbeitsweise des Skripts.

### 1. Python-Umgebung erstellen (Virtual Environment)

Zuerst erstellen wir einen isolierten Ordner (`.venv`), der die Python-Installation und Bibliotheken für dieses Projekt enthält.

Öffne dein Terminal im Hauptverzeichnis deines Projekts. Führe folgende Befehle nacheinander aus:

```bash
# 1. Virtuelle Umgebung erstellen
python3 -m venv .venv

# 2. Virtuelle Umgebung aktivieren
# (Dein Prompt ändert sich danach, meist steht (.venv) davor)
source .venv/bin/activate

```

*Herleitung:*
Das System-Python auf macOS sollte sauber bleiben. Durch `venv` erzeugen wir eine lokale Kopie der Python-Executable und der `site-packages`. Nur in dieser aktiven Umgebung installieren wir Bibliotheken.

### 2. Abhängigkeiten installieren

Das Skript importiert `pypdf` (`from pypdf import PdfReader...`). Diese Bibliothek ist standardmäßig nicht installiert. Wir aktualisieren zudem `pip`, um Warnmeldungen zu vermeiden.

```bash
# Pip aktualisieren (optional, aber empfohlen)
pip install --upgrade pip

# Installation der benötigten Bibliothek im aktiven Environment
pip install pypdf

```

### 3. Skript ausführen

Das Skript liegt nun im Unterordner `scripts/` und wurde so angepasst, dass es Verzeichnisse rekursiv durchsucht.

Wir übergeben den **Punkt (`.`)** als Argument. Dieser steht in Unix-Systemen für "das aktuelle Verzeichnis".

```bash
# Syntax: python [Pfad_zum_Skript] [Zielverzeichnis]
python scripts/pdf_splitter.py .

```

**Was jetzt passiert:**

1. Python lädt das Skript aus dem Ordner `scripts`.
2. Das Skript startet im aktuellen Ordner (`.`) und durchsucht alle Unterordner (`daten-speichern...`, `esp32...`, etc.).
3. Es ignoriert Systemordner wie `.venv` oder bereits erstellte `output_chapters`.
4. Sobald es eine PDF-Datei findet, prüft es auf Lesezeichen.
5. Es erstellt direkt neben der Original-PDF einen Ordner `output_chapters` und speichert die extrahierten Kapitel dort ab.

---

### Zusammenfassung der Befehlskette

Hier ist der komplette Ablauf für Copy & Paste in dein Terminal (vom Start bis zum Ergebnis-Check):

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install pypdf
python scripts/pdf_splitter.py .

# Ergebnisprüfung
tree -L 3
deactivate

```

### Fehlerbehandlung

* **Keine Lesezeichen:** Falls das Skript bei einer Datei meldet: `Hinweis: Dieses PDF enthält keine Lesezeichen (Outlines)`, wird diese Datei übersprungen. Das Skript läuft jedoch weiter und bearbeitet die restlichen Ordner.
* **Pfad nicht gefunden:** Stelle sicher, dass du dich im Hauptverzeichnis befindest (`ls` sollte die Ordner `scripts` und die Buch-Ordner anzeigen).
