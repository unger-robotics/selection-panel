# Markdown → HTML (Pandoc) für Projekt-Doku

Dieses Repository enthält eine kleine Build-Pipeline, die Markdown-Dateien aus `doc/md/` nach HTML in `doc/html/_site/` konvertiert – inkl. **Mermaid** (Diagramme) und **MathJax** (TeX-Formeln). Zusätzlich wird eine **Startseite** `doc/html/start.html` mit Link-Übersicht generiert.

## Struktur

```text
projekt/
├── doc/
│   ├── md/                 # Markdown-Quellen (*.md)
│   └── html/
│       ├── styles.css      # CSS-Quelle (SSOT; wird durch das Script erweitert)
│       ├── start.html      # generiert (Link-Übersicht)
│       └── _site/
│           ├── *.html      # generierte HTML-Seiten
│           └── styles.css  # Kopie der SSOT-CSS für _site/*.html
└── scripts/
    └── md_to_html_converter.py
```

## Anforderungen

* **Python 3**
* **Pandoc** muss installiert sein (`pandoc --version`)
* Internetzugang optional (Mermaid/MathJax werden per CDN eingebunden)

## Quickstart

### 1) HTML generieren

Im Projektroot:

```bash
./scripts/md_to_html_converter.py
```

Ergebnis:

* `doc/html/start.html`
* `doc/html/_site/*.html`

### 2) Lokal ansehen (macOS)

Direkt öffnen:

```bash
open doc/html/start.html
```

Robuster über lokalen Webserver (empfohlen):

```bash
cd doc/html
python3 -m http.server 8000
# Browser: http://localhost:8000/start.html
```

## CSS: Was wird verwendet?

* **SSOT (Quelle):** `doc/html/styles.css`
  Das Script ergänzt diese Datei idempotent um einen Block:
  `/* === MD-TO-HTML-CONVERTER EXTENSIONS === */ ...`

* **Für generierte Seiten:** `doc/html/_site/styles.css`
  Das Script kopiert die SSOT-CSS nach `_site/`, damit die generierten Seiten
  `styles.css` im selben Ordner referenzieren können.

Wichtig: Wenn du `doc/html/styles.css` manuell änderst, lass das Script einmal laufen,
damit `_site/styles.css` wieder synchron ist.

## Mermaid & MathJax

* Mermaid wird automatisch eingebunden, wenn in einer Datei ein Mermaid-Codeblock erkannt wird.
* TeX-Formeln werden via MathJax gerendert (Inline: `$...$`, Display: `$$...$$` / `\[...\]`).

## CLI-Optionen

```bash
./scripts/md_to_html_converter.py --help
```

Typische Beispiele:

Nur bestimmte Dateien konvertieren:

```bash
./scripts/md_to_html_converter.py doc/md/projekt.md doc/md/Serial-Protokoll.md
```

Rekursiv in Unterordnern suchen:

```bash
./scripts/md_to_html_converter.py --recursive
```

Eigene Quellordner angeben (Komma-separiert, mehrfach möglich):

```bash
./scripts/md_to_html_converter.py --dirs doc/md,README_SNIPPETS --recursive
```

Keine Startseite erzeugen:

```bash
./scripts/md_to_html_converter.py --no-start-page
```

## Troubleshooting

### 404 für `/favicon.ico`

Unkritisch. Browser fragt automatisch nach. Optional kannst du ein `favicon.ico` in `doc/html/` ablegen.

### Pandoc fehlt

Installieren und prüfen:

```bash
pandoc --version
```

### CSS wirkt “nicht aktuell”

Script erneut laufen lassen, damit `doc/html/_site/styles.css` aktualisiert wird.

## Lizenz

MIT License – siehe [`LICENSE`](../../LICENSE).

