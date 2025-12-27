#!/usr/bin/env python3

r"""
md_to_html_converter.py — Markdown → HTML mit Mermaid- & MathJax-Support

Version: 2.2.3 | Stand: 2025-12-26
Farbschema: Arduino Teal + Raspberry Pi Red (konsistent mit farbschema.tex)

Zweck
-----
Konvertiert Markdown-Dateien (*.md) in eigenständige HTML-Seiten. Unterstützt
Mermaid-Diagramme und TeX-Formeln via MathJax. Optional wird eine start.html
mit Link-Übersicht erzeugt.

Verzeichnisstruktur
-------------------
  projekt/
  ├── docs/
  │   ├── *.md              ← Quellen
  │   └── _site/
  │       ├── *.html        ← generiert
  │       └── styles.css
  ├── scripts/
  │   └── md-to-html-converter.py
  ├── styles.css       ← Quelle
  └── start.html            ← generiert

Aufruf
------
  cd projekt
  python3 scripts/md-to-html-converter.py

CLI-Optionen
------------
- files (positional): Liste von .md-Dateien (optional).
- --dirs: Kommaseparierte Verzeichnisse (Standard: docs).
- --recursive: rekursive Suche nach *.md.
- --no-start-page: unterdrückt die Generierung von start.html.
- -v / --verbose: ausführlichere Konsolenmeldungen.

Exit-Codes
----------
- 0   Erfolg
- 1   unerwarteter Fehler
- 130 Abbruch durch SIGINT (Strg+C)
"""

# === PFAD-KONFIGURATION ===
SOURCE_DIR = "docs"                # Markdown-Quelle
OUTPUT_DIR = "docs/_site"          # HTML-Zielverzeichnis
START_PAGE = "start.html"          # Startseite im Projektroot
CSS_FILE   = "styles.css"     # CSS im Projektroot

# Standardbibliothek
import argparse
import datetime
import glob
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from shutil import which
import html
import shutil


def check_pandoc():
    """Prüft, ob Pandoc installiert ist."""
    try:
        if not which('pandoc'):
            return False
        subprocess.run(['pandoc', '--version'],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
        return True
    except Exception:
        return False


def create_main_enhanced_filter(target_dir: Path):
    """Erstellt Lua-Filter im angegebenen Verzeichnis (temporär)."""
    lua_content = r"""-- main_enhanced_filter.lua – kompatibel ohne pandoc.List

-- Attribute/Classes sicherstellen (als einfache Tabellen)
local function ensure_attr(el)
  if not el.attr then el.attr = pandoc.Attr() end
  if not el.attr.classes then el.attr.classes = {} end
end

local function has_class(el, cls)
  ensure_attr(el)
  for _, c in ipairs(el.attr.classes) do
    if c == cls then return true end
  end
  return false
end

local function add_class(el, cls)
  ensure_attr(el)
  if not has_class(el, cls) then
    table.insert(el.attr.classes, cls)
  end
end

-- Tabellen: Klassen ergänzen (duplikatfrei)
function Table(tbl)
  add_class(tbl, 'table')
  add_class(tbl, 'table-compact')
  return tbl
end

-- CodeBlock: Mermaid unverändert lassen
function CodeBlock(cb)
  ensure_attr(cb)
  for _, c in ipairs(cb.attr.classes) do
    if string.lower(c) == 'mermaid' then
      return cb
    end
  end
  return cb
end

-- Inline-Math kapseln; DisplayMath unverändert
function Math(el)
  if el.mathtype == 'InlineMath' then
    local html = '<span class="math math-inline">$' .. el.text .. '$</span>'
    return pandoc.RawInline('html', html)
  end
  return el
end

-- Prüft, ob Block ausschließlich EIN DisplayMath enthält
local function sole_display_math_block(blk)
  if blk.t ~= 'Para' and blk.t ~= 'Plain' then return nil end
  local inlines = blk.content
  if #inlines ~= 1 then return nil end
  local x = inlines[1]
  if x.t == 'Math' and x.mathtype == 'DisplayMath' then
    return x.text
  end
  return nil
end

-- Reine DisplayMath-Absätze → HTML-Block
function Blocks(blocks)
  local out = {}
  for _, blk in ipairs(blocks) do
    local dm = sole_display_math_block(blk)
    if dm then
      table.insert(out, pandoc.RawBlock('html', '<div class="math math-display">$$' .. dm .. '$$</div>'))
    else
      table.insert(out, blk)
    end
  end
  return out
end

-- Hauptcontainer nur einmal einfügen
local main_container_start = '<div class="main-container">'
local main_container_end   = '</div>'

function Pandoc(doc)
  local first = doc.blocks[1]
  local last  = doc.blocks[#doc.blocks]
  local has_start = first and first.t == 'RawBlock' and first.format == 'html'
                    and first.text:match('main%-container')
  local has_end   = last and last.t == 'RawBlock' and last.format == 'html'
                    and last.text:match('^%s*</div>%s*$')
  if not has_start then
    table.insert(doc.blocks, 1, pandoc.RawBlock('html', main_container_start))
  end
  if not has_end then
    table.insert(doc.blocks, pandoc.RawBlock('html', main_container_end))
  end
  return doc
end

return {
  Table    = Table,
  CodeBlock= CodeBlock,
  Math     = Math,
  Blocks   = Blocks,
  Pandoc   = Pandoc
}
"""
    filter_path = target_dir / 'main_enhanced_filter.lua'
    with open(filter_path, 'w', encoding='utf-8') as f:
        f.write(lua_content)
    return filter_path


def enhance_css_file():
    """Erweitert/aktualisiert 'styles.css' idempotent um Regeln für Mermaid/Charts/Math."""
    css_path = Path(CSS_FILE)
    if not css_path.exists():
        print(f"Warnung: CSS-Datei '{CSS_FILE}' nicht gefunden.")
        return

    marker_begin = "/* === MD-TO-HTML-CONVERTER EXTENSIONS === */"
    marker_end   = "/* === END MD-TO-HTML-CONVERTER EXTENSIONS === */"

    additional_css = f"""
{marker_begin}

/* Mermaid-Diagramme */
.mermaid {{
  text-align: center;
  margin: 1.5em 0;
}}
pre.mermaid {{
  background: transparent;
  border: none;
  padding: 0;
}}

/* Math-Formeln */
.math-inline {{
  display: inline;
}}
.math-display {{
  display: block;
  text-align: center;
  margin: 1.5em 0;
  overflow-x: auto;
}}

/* Tabellen */
.table {{
  width: 100%;
  border-collapse: collapse;
  margin: 1em 0;
}}
.table th, .table td {{
  border: 1px solid #E1E4E8;
  padding: 0.5em;
  text-align: left;
}}

/* Tabellenkopf: Arduino Dark + weiße Schrift */
.table thead th,
.table tr:first-child th {{
  background-color: #005C5F;
  color: #ffffff;
}}
.table thead {{
  border-bottom: 2px solid #005C5F;
}}

.table-compact th, .table-compact td {{
  padding: 0.3em 0.5em;
}}

/* Responsive: Große Bildschirme */
@media (min-width: 1400px) {{
  .table th, .table td {{ padding: 0.6em 0.75em; }}
}}
@media (min-width: 1920px) {{
  .table {{ font-size: 1rem; }}
  .table th, .table td {{ padding: 0.7em 0.85em; }}
}}

{marker_end}
""".strip() + "\n"

    content = css_path.read_text(encoding="utf-8")

    # Falls Block existiert: ersetzen (idempotent + updatefähig)
    if marker_begin in content and marker_end in content:
        pattern = re.compile(
            re.escape(marker_begin) + r".*?" + re.escape(marker_end),
            flags=re.DOTALL
        )
        new_content, n = pattern.subn(additional_css.strip(), content, count=1)
        if n:
            css_path.write_text(new_content + ("\n" if not new_content.endswith("\n") else ""), encoding="utf-8")
            print(f"✓ CSS Extensions aktualisiert: {CSS_FILE}")
        return

    # Falls Marker kaputt/teilweise vorhanden: Block hinten neu anhängen
    if marker_begin in content and marker_end not in content:
        print(f"Warnung: Marker-Bereich in '{CSS_FILE}' unvollständig. Hänge neuen Block an.")
        content = content.rstrip() + "\n\n" + additional_css
        css_path.write_text(content, encoding="utf-8")
        print(f"✓ CSS erweitert: {CSS_FILE}")
        return

    # Normalfall: anhängen
    with open(css_path, "a", encoding="utf-8") as f:
        f.write("\n" + additional_css)
    print(f"✓ CSS erweitert: {CSS_FILE}")


def check_and_fix_content(markdown_files):
    """
    Scannt Markdown-Dateien auf Mermaid-/Math-Inhalte.
    Kapselt rohe Mermaid-Blöcke idempotent in ```mermaid ... ```.
    """
    mermaid_re = re.compile(r'```\s*mermaid', re.IGNORECASE)
    raw_mermaid_re = re.compile(
        r'^(graph|flowchart)\s+(TD|TB|LR|RL|BT)\b',
        re.MULTILINE
    )
    math_re = re.compile(r'\$[^$]+\$|\\\[|\\\(|\\begin\{')

    files_with_mermaid = []
    files_with_math = []
    files_updated = []

    for md_file in markdown_files:
        try:
            with open(md_file, 'r', encoding='utf-8') as f:
                content = f.read()
        except Exception as e:
            print(f"Warnung: Kann '{md_file}' nicht lesen: {e}")
            continue

        has_mermaid = bool(mermaid_re.search(content))
        has_math = bool(math_re.search(content))

        # Rohe Mermaid-Blöcke kapseln
        if raw_mermaid_re.search(content) and not mermaid_re.search(content):
            def wrap_mermaid(m):
                return f"```mermaid\n{m.group(0)}"
            new_content = raw_mermaid_re.sub(wrap_mermaid, content)
            if new_content != content:
                with open(md_file, 'w', encoding='utf-8') as f:
                    f.write(new_content)
                files_updated.append(md_file)
                has_mermaid = True

        if has_mermaid:
            files_with_mermaid.append(md_file)
        if has_math:
            files_with_math.append(md_file)

    return files_with_mermaid, files_with_math, files_updated


def sanitize_math_for_mathjax(text: str) -> str:
    """
    Konservative Auto-Korrekturen für häufige Pandoc/MathJax-Warnungen.
    """
    if not text:
        return text

    fenced_re = re.compile(r"^\s*(```|~~~)")
    inline_code_re = re.compile(r"(`+)(.+?)\1")
    url_re = re.compile(r"https?://")
    has_math_delim = lambda s: ("$" in s) or (r"\[" in s) or (r"\(" in s)
    tex_marker_re = re.compile(
        r"\\(frac|xrightarrow|dot|ddot|Delta|eta|rho|mathrm|sum|int|approx|sqrt|cdot|overline|underline|vec|bar)\b"
    )
    unit_token_re = re.compile(r"\\text\{([A-Za-zµΩ]{1,4})\}")

    in_fence = False
    out_lines = []

    for raw_line in text.splitlines():
        line = raw_line

        if fenced_re.match(line):
            in_fence = not in_fence
            out_lines.append(line)
            continue

        if not in_fence:
            line = line.replace("{,}", ",")

        def _units_text_to_rm(m):
            token = m.group(1)
            return r"\mathrm{" + token + "}"

        if not in_fence:
            line = unit_token_re.sub(_units_text_to_rm, line)

        if (not in_fence
            and not has_math_delim(line)
            and not inline_code_re.search(line)
            and not url_re.search(line)
            and tex_marker_re.search(line)):
            line = f"${line}$"

        out_lines.append(line)

    return "\n".join(out_lines)


def create_mermaid_header(target_dir: Path) -> Path:
    """Erstellt Mermaid-Header im Zielverzeichnis."""
    content = r"""
<script src="https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js"></script>
<script>
  document.addEventListener('DOMContentLoaded', function() {
    mermaid.initialize({
      startOnLoad: true,
      theme: 'default',
      flowchart: {
        useMaxWidth: true,
        htmlLabels: true,
        curve: 'basis',
        nodeSpacing: 50,
        rankSpacing: 70
      }
    });
  });
</script>
"""
    path = target_dir / 'mermaid-header.html'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    return path


def create_mathjax_header(target_dir: Path) -> Path:
    """Erstellt MathJax-Header im Zielverzeichnis."""
    content = r"""
<script>
window.MathJax = {
  loader: {
    load: ['[tex]/ams']
  },
  tex: {
    inlineMath: [['$','$'], ['\\(','\\)']],
    displayMath: [['$$','$$'], ['\\[','\\]']],
    processEscapes: true,
    processEnvironments: true,
    packages: {'[+]': ['ams']},
    macros: {
      dd: '{\\,\\mathrm{d}}',
      degC: '{^{\\circ}\\!\\mathrm{C}}',
      Ohm: '{\\,\\Omega}',
      ohm: '{\\,\\Omega}',
      degree: '{^{\\circ}}'
    }
  },
  options: {
    skipHtmlTags: ['script','noscript','style','textarea','pre','code','kbd','samp']
  }
};
</script>
<script defer src="https://cdn.jsdelivr.net/npm/mathjax@3.2.2/es5/tex-mml-chtml.js"></script>
"""
    path = target_dir / 'mathjax-header.html'
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    return path


def _normalize_dirs(dir_list):
    """Normalisiert und validiert Verzeichnisliste."""
    seen = set()
    out = []
    for d in dir_list or []:
        dd = str(d).strip().strip("/").replace("\\", "/")
        if not dd or dd in seen:
            continue
        if os.path.isdir(dd):
            out.append(dd)
            seen.add(dd)
    return out


def ensure_css_in_dir(target_dir: Path, css_file: str):
    """Kopiert CSS in target_dir, falls es dort fehlt oder älter ist."""
    try:
        target_dir.mkdir(parents=True, exist_ok=True)
        src = Path(css_file)
        if not src.exists():
            print(f"Warnung: CSS-Quelle '{css_file}' nicht gefunden.")
            return
        dst = target_dir / src.name
        if (not dst.exists()) or (src.stat().st_mtime > dst.stat().st_mtime + 1e-6):
            shutil.copy2(src, dst)
    except Exception as e:
        print(f"Warnung: CSS konnte nicht nach '{target_dir}' gespiegelt werden: {e}")


def _natkey(s: str):
    """Natürliche Sortierung: 'file2' < 'file10'."""
    parts = re.findall(r'\d+|\D+', str(s))
    return [int(t) if t.isdigit() else t.lower() for t in parts]


def generate_start_page(html_files: list):
    """
    Erstellt start.html im Projektroot mit Links zu allen HTMLs in docs/_site/.
    """
    if not html_files:
        print("Keine HTML-Dateien für die Startseite gefunden.")
        return

    current_time = datetime.datetime.now().strftime("%d.%m.%Y %H:%M")

    html_content = [
        "<!DOCTYPE html>",
        '<html lang="de">',
        "<head>",
        '  <meta charset="UTF-8">',
        '  <meta name="viewport" content="width=device-width, initial-scale=1.0">',
        "  <title>Dokumentation - Startseite</title>",
        f'  <link rel="stylesheet" href="{OUTPUT_DIR}/{CSS_FILE}">',
        "</head>",
        "<body>",
        '  <div class="main-container">',
        "    <h1>Dokumentation</h1>",
        '    <ul class="documentation-list">',
    ]

    for file_path in sorted(html_files, key=_natkey):
        path_norm = str(file_path).replace("\\", "/")
        title = Path(path_norm).stem.replace('-', ' ').replace('_', ' ')
        html_content.append(
            f'      <li><a href="{html.escape(path_norm)}">{html.escape(title)}</a></li>'
        )

    html_content += [
        "    </ul>",
        "    <footer>",
        f"      <p>bearbeitet am {html.escape(current_time)}</p>",
        "    </footer>",
        "  </div>",
        "</body>",
        "</html>",
    ]

    with open(START_PAGE, 'w', encoding='utf-8') as f:
        f.write("\n".join(html_content))
    print(f"✓ Startseite '{START_PAGE}' erfolgreich erstellt")


def convert_all_markdown_files(args):
    """Konvertiert alle *.md-Dateien mithilfe von Pandoc und dem Lua-Filter."""
    if not check_pandoc():
        print("Fehler: Pandoc ist nicht installiert.")
        return

    # Output-Verzeichnis erstellen
    output_path = Path(OUTPUT_DIR)
    output_path.mkdir(parents=True, exist_ok=True)

    # Temporäre Dateien im Output-Verzeichnis erstellen
    lua_filter = create_main_enhanced_filter(output_path)
    enhance_css_file()

    if not os.path.exists(CSS_FILE):
        print(f"Warnung: CSS-Datei '{CSS_FILE}' nicht gefunden.")

    # --- Verzeichnisse bestimmen ---
    md_dirs = [SOURCE_DIR]
    if args.dirs:
        user_dirs = []
        for spec in args.dirs:
            user_dirs.extend([p.strip() for p in spec.split(",") if p.strip()])
        md_dirs = user_dirs if user_dirs else [SOURCE_DIR]

    md_dirs = _normalize_dirs(md_dirs)
    if not md_dirs:
        print("Keine gültigen Markdown-Verzeichnisse gefunden.")
        return

    # Sammle alle Markdown-Dateien (ohne _site/)
    if args.files:
        markdown_files = [f for f in args.files if f.endswith('.md') and os.path.isfile(f)]
    else:
        markdown_files = []
        pattern = "**/*.md" if args.recursive else "*.md"
        for md_dir in md_dirs:
            if os.path.isdir(md_dir):
                for f in glob.glob(os.path.join(md_dir, pattern), recursive=args.recursive):
                    # _site/ ausschließen
                    if '_site' not in f:
                        markdown_files.append(f)

    if not markdown_files:
        print("Keine Markdown-Dateien zum Konvertieren gefunden.")
        return

    print(f"Gefundene Markdown-Dateien: {len(markdown_files)}")

    files_with_mermaid, files_with_math, files_updated = check_and_fix_content(markdown_files)
    if files_with_mermaid:
        print(f"Dateien mit Mermaid-Diagrammen: {len(files_with_mermaid)}")
    if files_with_math:
        print(f"Dateien mit TeX-Formeln: {len(files_with_math)}")
    if files_updated:
        print(f"Syntax in {len(files_updated)} Dateien korrigiert")

    # CSS ins Zielverzeichnis kopieren
    ensure_css_in_dir(output_path, CSS_FILE)

    # Header-Dateien erstellen
    mermaid_header = create_mermaid_header(output_path)
    mathjax_header = create_mathjax_header(output_path)

    html_files = []

    for md_file in markdown_files:
        # HTML nach _site/ schreiben (flache Struktur)
        html_name = Path(md_file).stem + ".html"
        html_file = output_path / html_name

        # Optional: Eingabe sanitizen → temp-Datei
        tmp_input = md_file
        try:
            with open(md_file, 'r', encoding='utf-8') as f:
                raw = f.read()
            sanitized = sanitize_math_for_mathjax(raw)
            if sanitized != raw:
                tmp_input = str(Path(md_file).with_suffix('.mathjax_tmp.md'))
                with open(tmp_input, 'w', encoding='utf-8') as f:
                    f.write(sanitized)
        except Exception:
            tmp_input = md_file

        cmd = [
            'pandoc',
            tmp_input,
            '-o', str(html_file),
            '--standalone',
            '--to=html5',
            '--from=markdown+tex_math_dollars+tex_math_single_backslash+raw_tex',
            '--mathjax',
            '--css', CSS_FILE,
            f'--lua-filter={lua_filter}'
        ]
        if md_file in files_with_mermaid:
            cmd.append(f'--include-in-header={mermaid_header}')
        if md_file in files_with_math:
            cmd.append(f'--include-in-header={mathjax_header}')

        try:
            print(f"Konvertiere {md_file} → {html_file}...")
            subprocess.run(cmd, check=True)
            print(f"✓ Erfolgreich: {html_file}")
            html_files.append(str(html_file))

        except subprocess.CalledProcessError as e:
            print(f"✗ Fehler bei {md_file}: {e}")
        finally:
            if tmp_input != md_file and os.path.exists(tmp_input):
                os.remove(tmp_input)

    # Temporäre Header-Dateien aufräumen
    for temp_file in [mermaid_header, mathjax_header, lua_filter]:
        if temp_file.exists():
            temp_file.unlink()

    if not args.no_start_page:
        generate_start_page(html_files)


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Konvertiert Markdown-Dateien zu HTML (Mermaid + MathJax).'
    )
    parser.add_argument(
        'files', nargs='*',
        help='Spezifische Markdown-Dateien (optional).'
    )
    parser.add_argument(
        '--no-start-page', action='store_true',
        help='Keine Startseite generieren.'
    )
    parser.add_argument(
        '--verbose', '-v', action='store_true',
        help='Ausführliche Ausgabe.'
    )
    parser.add_argument(
        '--dirs', action='append',
        help=f'Komma-separierte Verzeichnisse (Standard: {SOURCE_DIR}).'
    )
    parser.add_argument(
        '--recursive', action='store_true',
        help='Rekursiv in Unterordnern nach *.md suchen.'
    )

    args = parser.parse_args()

    try:
        convert_all_markdown_files(args)
        return 0
    except KeyboardInterrupt:
        print("Abgebrochen (Strg+C).")
        return 130
    except Exception as e:
        print(f"Unerwarteter Fehler: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
