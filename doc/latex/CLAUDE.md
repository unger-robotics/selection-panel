# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build-Befehle

```bash
make              # Alle PDFs bauen (main + covers)
make publish      # PDFs nach assets/pdf kopieren
make clean        # Temp-Dateien löschen
make distclean    # build/ komplett löschen
make debug        # Erkannte Dateien anzeigen
```

Einzelne Dokumente werden automatisch bei Änderungen neu gebaut. Build-Ausgabe in `build/`.

**Voraussetzung:** XeLaTeX + latexmk (TeX Live 2025 empfohlen)

## Verzeichnisstruktur

```
latex/
├── main/           # Hauptdokumente (main_*.tex, praesentation.tex)
├── content/        # Modulare Kapitel (HARDWARE.tex, PROTOCOL.tex, ...)
├── covers/         # Titelseiten (cover-*.tex)
├── class/          # techdoc.cls Template-Klasse
├── images/         # Bilder (PNG, PDF)
└── build/          # Generierte PDFs + Temp-Dateien
```

## Dokumenten-Architektur

**Hauptdokumente** in `main/` inkludieren Kapitel aus `content/` via `\input{}`:

- `main_selection-panel.tex` – Vollständige Projektdokumentation
- `main_schieberegister.tex` – Schieberegister-Theorie
- `main_farbschema.tex` – Farbschema-Dokumentation
- `praesentation.tex` – Beamer-Präsentation (inkludiert `content/folien.tex`)

**Content-Fragmente** sind modulare Kapitel ohne `\documentclass`:
- Beginnen mit `\section{}`, nicht mit Präambel
- Können in mehrere Hauptdokumente eingebunden werden
- Verwenden `\Cref{}` für Querverweise

## techdoc.cls Template

Die Klasse basiert auf `article` und bietet:

### Listing-Styles
- `arduino` – C++ mit Arduino-Keywords (Default)
- `python` – Python mit async/await
- `shell` – Bash-Befehle
- `json` – JSON mit Syntax-Highlighting

```latex
\begin{lstlisting}[style=shell]
make && make publish
\end{lstlisting}
```

### Info-Boxen
```latex
\begin{infobox}[Titel]
  Hinweis-Text
\end{infobox}

\begin{warnbox}[Warnung]
  Warnungs-Text
\end{warnbox}

\begin{tipbox}[Tipp]
  Tipp-Text
\end{tipbox}
```

### Makros
- `\code{text}` – Inline-Code
- `\highlight{text}` – ArduinoTeal hervorgehoben
- `\warning{text}` – Orange Warnung
- `\critical{text}` – Rot kritisch
- `\SI{100}{\milli\second}` – SI-Einheiten
- `\Cref{sec:label}` – Intelligente Querverweise
- `\glqq` / `\grqq` – Deutsche Anführungszeichen

### Klassenoptionen
```latex
\documentclass{techdoc}           % Standard (farbige Links)
\documentclass[print]{techdoc}    % Druck (keine farbigen Links)
\documentclass[draft]{techdoc}    % Entwurf (todonotes aktiv)
```

## Neues Kapitel erstellen

1. Datei `content/NEUES-KAPITEL.tex` anlegen:
```latex
% =============================================================================
% NEUES-KAPITEL.tex – Beschreibung
% Modulares Fragment (kein \documentclass, kein \begin{document})
% Stand: YYYY-MM-DD | Version: X.Y
% =============================================================================

\section{Kapitelname}
\label{sec:neues-kapitel}

Inhalt...
```

2. In Hauptdokument einbinden (`main/main_*.tex`):
```latex
\input{content/NEUES-KAPITEL.tex}
```

## Häufige LaTeX-Konstrukte

### Tabellen
```latex
\begin{table}[H]
  \centering
  \caption{Beschreibung}
  \label{tab:beispiel}
  \begin{tabularx}{\textwidth}{@{}l X r@{}}
    \toprule
    \textbf{Spalte 1} & \textbf{Spalte 2} & \textbf{Wert} \\
    \midrule
    Zeile 1 & Beschreibung & \SI{100}{\ohm} \\
    \bottomrule
  \end{tabularx}
\end{table}
```

### Bilder
```latex
\begin{figure}[H]
  \centering
  \includegraphics[width=0.8\textwidth]{images/bild.png}
  \caption{Beschreibung}
  \label{fig:beispiel}
\end{figure}
```

## Farbschema

| Farbe | Hex | Verwendung |
|-------|-----|------------|
| ArduinoTeal | #00979D | Links, Highlights |
| ArduinoDark | #005C5F | Keywords |
| RaspberryRed | #C51A4A | Strings, Critical |
| SuccessGreen | #75A928 | Tipbox |
| WarningOrange | #D97706 | Warnbox |

## Labels eindeutig halten

Labels müssen dokumentenweit eindeutig sein. Konvention:
- `sec:kapitelname` – Sections
- `subsec:kapitel-unterkapitel` – Subsections
- `tab:kapitel-beschreibung` – Tabellen
- `fig:kapitel-beschreibung` – Abbildungen
- `lst:kapitel-beschreibung` – Listings
