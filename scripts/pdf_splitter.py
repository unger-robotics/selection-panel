import os
import re
import sys
from pypdf import PdfReader, PdfWriter

def sanitize_filename(name):
    """Bereinigt den Titel für Dateinamen."""
    clean_name = re.sub(r'[\\/*?:"<>|]', "_", name)
    clean_name = clean_name.replace('\n', ' ').replace('\r', '')
    return clean_name.strip()[:100]

def split_pdf(input_path):
    """
    Splittet ein einzelnes PDF.
    Der Ausgabeordner wird automatisch im gleichen Verzeichnis wie das PDF erstellt.
    """
    # Verzeichnis und Dateiname trennen
    base_dir = os.path.dirname(input_path)
    filename = os.path.basename(input_path)
    output_dir = os.path.join(base_dir, "output_chapters")

    print(f"\n--- Verarbeite: {filename} ---")

    if not os.path.exists(input_path):
        print(f"Fehler: Datei nicht gefunden.")
        return

    try:
        reader = PdfReader(input_path)
        outlines = reader.outline
    except Exception as e:
        print(f"Fehler/Keine Outlines: {e}")
        return

    # Kapitel extrahieren
    chapters = []
    for item in outlines:
        if isinstance(item, list): continue # Keine Unterkapitel
        try:
            page_num = reader.get_destination_page_number(item)
            if page_num is not None:
                chapters.append({"title": item.title, "start": page_num})
        except: pass

    if not chapters:
        print("-> Keine Hauptkapitel gefunden. Überspringe Datei.")
        return

    # Sortieren und Output-Ordner erstellen
    chapters.sort(key=lambda x: x["start"])

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        print(f"-> Ordner erstellt: {output_dir}")

    total_pages = len(reader.pages)

    # Splitten
    for i, chapter in enumerate(chapters):
        start = chapter["start"]
        end = chapters[i+1]["start"] if i+1 < len(chapters) else total_pages

        if start >= end: continue

        writer = PdfWriter()
        for p in range(start, end):
            writer.add_page(reader.pages[p])

        safe_title = sanitize_filename(chapter["title"])
        fname = f"{i+1:02d}_{safe_title}.pdf"

        with open(os.path.join(output_dir, fname), "wb") as f:
            writer.write(f)

    print(f"-> Fertig: {len(chapters)} Kapitel extrahiert.")

def process_path(target):
    """Entscheidet, ob Datei oder Verzeichnis verarbeitet wird."""

    # Fall 1: Ziel ist eine direkte Datei
    if os.path.isfile(target) and target.lower().endswith(".pdf"):
        split_pdf(target)
        return

    # Fall 2: Ziel ist ein Ordner (Rekursion)
    for root, dirs, files in os.walk(target):
        # Ignoriere Output-Ordner und versteckte Ordner (z.B. .venv, .git)
        if "output_chapters" in root or "/." in root:
            continue

        for file in files:
            if file.lower().endswith(".pdf"):
                # Ignoriere bereits gesplittete Kapitel (Starten oft mit Ziffern)
                # und vermeide rekursives Splitten von Split-Dateien
                if root.endswith("output_chapters"):
                    continue

                full_path = os.path.join(root, file)
                split_pdf(full_path)

if __name__ == "__main__":
    # Standard: Aktuelles Verzeichnis (.), wenn nichts angegeben
    target = sys.argv[1] if len(sys.argv) > 1 else "."
    process_path(target)
