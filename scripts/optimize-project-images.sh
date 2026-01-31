#!/bin/bash
# =============================================================================
# optimize-project-images.sh - Alle PNG-Dateien im Projekt optimieren
# =============================================================================
# Durchsucht alle Unterordner und optimiert PNG-Dateien verlustfrei
# Kompatibel mit macOS bash 3.2
#
# Verwendung:
#   ./optimize-project-images.sh              # Trockenlauf (zeigt nur an)
#   ./optimize-project-images.sh --run        # Wirklich ausführen
#   ./optimize-project-images.sh --run --max-width 2400
#
# Autor: Jan Unger | Stand: 2025-12-27
# =============================================================================

set -e

# Konfiguration
MAX_WIDTH=1920
DRY_RUN=true
BACKUP_DIR="originals_backup"
MIN_SIZE_KB=500  # Nur Dateien > 500 KB optimieren

# Argumente
while [[ $# -gt 0 ]]; do
    case $1 in
        --run) DRY_RUN=false; shift ;;
        --max-width) MAX_WIDTH="$2"; shift 2 ;;
        -h|--help)
            echo "Verwendung: $0 [--run] [--max-width PIXEL]"
            echo ""
            echo "  --run           Wirklich ausführen (sonst nur Vorschau)"
            echo "  --max-width N   Maximale Breite (Standard: 1920)"
            exit 0
            ;;
        *) shift ;;
    esac
done

# Helfer
format_size() {
    local bytes=$(stat -f%z "$1" 2>/dev/null)
    if [[ $bytes -ge 1048576 ]]; then
        awk "BEGIN {printf \"%.1f MB\", $bytes / 1048576}"
    else
        awk "BEGIN {printf \"%.0f KB\", $bytes / 1024}"
    fi
}

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  Projekt-weite PNG-Optimierung                            ║"
echo "╠═══════════════════════════════════════════════════════════╣"
echo "║  Max. Breite:  ${MAX_WIDTH}px                                     ║"
echo "║  Min. Größe:   ${MIN_SIZE_KB} KB                                     ║"
if $DRY_RUN; then
echo "║  Modus:        VORSCHAU (--run zum Ausführen)             ║"
else
echo "║  Modus:        AUSFÜHREN                                  ║"
fi
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""

# oxipng prüfen
if command -v oxipng &>/dev/null; then
    echo "✓ oxipng installiert"
else
    echo "○ oxipng nicht installiert (nur Resize)"
    echo "  Installieren: brew install oxipng"
fi
echo ""

# Alle PNG-Dateien finden (ohne build/, out/, _site/, backup)
echo "Suche PNG-Dateien..."
echo ""

# Temporäre Datei für die zu optimierenden Dateien
tmp_file=$(mktemp)
tmp_all=$(mktemp)

# Alle PNG-Dateien sammeln
find . -type f -iname "*.png" \
    ! -path "./build/*" \
    ! -path "./out/*" \
    ! -path "./*/_site/*" \
    ! -path "./$BACKUP_DIR/*" \
    2>/dev/null | sort > "$tmp_all"

printf "%-50s %10s %10s %s\n" "DATEI" "GRÖSSE" "BREITE" "AKTION"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

while read -r file; do
    size_bytes=$(stat -f%z "$file" 2>/dev/null)
    size_kb=$((size_bytes / 1024))
    size_display=$(format_size "$file")
    width=$(sips -g pixelWidth "$file" 2>/dev/null | awk '/pixelWidth:/{print $2}')
    
    # Entscheiden ob optimieren
    action="übersprungen"
    if [[ $size_kb -ge $MIN_SIZE_KB ]]; then
        if [[ $width -gt $MAX_WIDTH ]]; then
            action="RESIZE ${width}→${MAX_WIDTH}"
            echo "$file" >> "$tmp_file"
        else
            action="OXIPNG"
            echo "$file" >> "$tmp_file"
        fi
    fi
    
    # Kürzen des Pfads für Anzeige
    short_path="${file:2}"
    if [[ ${#short_path} -gt 48 ]]; then
        short_path="...${short_path: -45}"
    fi
    
    printf "%-50s %10s %8spx %s\n" "$short_path" "$size_display" "$width" "$action"
done < "$tmp_all"

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Zählen
total_files=$(wc -l < "$tmp_all" | tr -d ' ')
optimize_files=$(wc -l < "$tmp_file" 2>/dev/null | tr -d ' ')

# Gesamtgröße berechnen
total_size=0
while read -r file; do
    size=$(stat -f%z "$file" 2>/dev/null)
    total_size=$((total_size + size))
done < "$tmp_all"
total_size_mb=$(awk "BEGIN {printf \"%.1f\", $total_size / 1048576}")

echo "Gesamt: $total_files Dateien, ${total_size_mb} MB"
echo "Zu optimieren: $optimize_files Dateien"
echo ""

if [[ ! -s "$tmp_file" ]]; then
    echo "Keine Dateien zum Optimieren."
    rm -f "$tmp_file" "$tmp_all"
    exit 0
fi

if $DRY_RUN; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "VORSCHAU - Zum Ausführen: $0 --run"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    rm -f "$tmp_file" "$tmp_all"
    exit 0
fi

# Ausführen
echo "Erstelle Backup in $BACKUP_DIR/..."
mkdir -p "$BACKUP_DIR"

while read -r file; do
    filename=$(basename "$file")
    dirname=$(dirname "$file")
    
    # Backup mit Pfad-Info
    backup_name="${dirname//\//_}_${filename}"
    backup_name="${backup_name#._}"
    cp "$file" "$BACKUP_DIR/$backup_name"
    
    orig_size=$(format_size "$file")
    width=$(sips -g pixelWidth "$file" 2>/dev/null | awk '/pixelWidth:/{print $2}')
    
    echo ""
    echo "📄 $file"
    
    # Resize
    if [[ $width -gt $MAX_WIDTH ]]; then
        echo "   Resize: ${width}px → ${MAX_WIDTH}px"
        sips --resampleWidth $MAX_WIDTH "$file" &>/dev/null
    fi
    
    # oxipng
    if command -v oxipng &>/dev/null; then
        echo "   oxipng: Kompression optimieren..."
        oxipng -o 4 --strip safe "$file" 2>/dev/null
    fi
    
    new_size=$(format_size "$file")
    echo "   ✓ $orig_size → $new_size"
    
done < "$tmp_file"

rm -f "$tmp_file"

# Nachher
total_after=0
while read -r file; do
    [[ -f "$file" ]] && size=$(stat -f%z "$file" 2>/dev/null) && total_after=$((total_after + size))
done < "$tmp_all"
total_after_mb=$(awk "BEGIN {printf \"%.1f\", $total_after / 1048576}")

rm -f "$tmp_all"

saved=$((total_size - total_after))
saved_mb=$(awk "BEGIN {printf \"%.1f\", $saved / 1048576}")

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  Zusammenfassung                                          ║"
echo "╠═══════════════════════════════════════════════════════════╣"
printf "║  Vorher:    %-10s                                  ║\n" "${total_size_mb} MB"
printf "║  Nachher:   %-10s                                  ║\n" "${total_after_mb} MB"
printf "║  Ersparnis: %-10s                                  ║\n" "${saved_mb} MB"
echo "║  Backup:    $BACKUP_DIR/                              ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "Wiederherstellen: cp $BACKUP_DIR/[datei] [ziel]"
