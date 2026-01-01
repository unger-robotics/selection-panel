#!/bin/bash
# ==============================================================================
# rename_sounds.sh - Benennt Pixabay-Downloads für Selection Panel
# ==============================================================================
#
# Verwendung:
#   ./rename_sounds.sh [VERZEICHNIS] [--auto]
# Script ausführen
# cd media/
# ../scripts/rename_sounds.sh --auto
#
# Erkennt automatisch alle MP3-Dateien und benennt sie zu 001.mp3 - 010.mp3
# Pixabay-Downloads haben Namen wie "news-background-music-144801.mp3"
#
# Optionen:
#   --auto, -a    Keine Bestätigung erfragen
#
# ==============================================================================

set -e

MEDIA_DIR="${1:-.}"
AUTO_MODE=false

# Parameter parsen
for arg in "$@"; do
    case $arg in
        --auto|-a)
            AUTO_MODE=true
            ;;
    esac
done

# Falls erstes Argument ein Flag ist, Verzeichnis auf . setzen
if [[ "$MEDIA_DIR" == "--auto" ]] || [[ "$MEDIA_DIR" == "-a" ]]; then
    MEDIA_DIR="."
fi

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "========================================"
echo "Selection Panel - Sound Rename Script"
echo "========================================"
echo ""

cd "$MEDIA_DIR" || {
    echo -e "${RED}Fehler: Verzeichnis '$MEDIA_DIR' nicht gefunden${NC}"
    exit 1
}

echo "Arbeitsverzeichnis: $(pwd)"
echo ""

# Alle MP3-Dateien finden (ausser bereits umbenannte 001-010)
shopt -s nullglob
mp3_files=()
for f in *.mp3; do
    # Überspringe bereits nummerierte Dateien (001.mp3 bis 099.mp3)
    if [[ ! "$f" =~ ^0[0-9][0-9]\.mp3$ ]]; then
        mp3_files+=("$f")
    fi
done
shopt -u nullglob

# Sortieren nach Name
IFS=$'\n' mp3_files=($(sort <<<"${mp3_files[*]}")); unset IFS

if [ ${#mp3_files[@]} -eq 0 ]; then
    echo -e "${YELLOW}Keine neuen MP3-Dateien gefunden.${NC}"
    echo ""
    echo "Vorhandene Dateien:"
    ls -lh *.mp3 2>/dev/null || echo "(keine)"
    exit 0
fi

echo -e "${CYAN}Gefundene MP3-Dateien (${#mp3_files[@]}):${NC}"
for i in "${!mp3_files[@]}"; do
    num=$((i+1))
    echo "  $(printf '%2d' $num). ${mp3_files[$i]}"
done
echo ""

# Nächste freie Nummer finden
next_num=1
while [ -f "$(printf '%03d.mp3' $next_num)" ] && [ $next_num -le 100 ]; do
    ((next_num++))
done

echo -e "${CYAN}Starte bei Nummer: $(printf '%03d' $next_num)${NC}"
echo ""

# Prüfen ob genug Platz (max 100)
end_num=$((next_num + ${#mp3_files[@]} - 1))
if [ $end_num -gt 100 ]; then
    echo -e "${RED}Warnung: Nur $((100 - next_num + 1)) Plätze frei (001-100)${NC}"
fi

# Bestätigung
if [ "$AUTO_MODE" = false ]; then
    echo "Umbenennen zu $(printf '%03d' $next_num).mp3 - $(printf '%03d' $end_num).mp3? (j/n)"
    read -r confirm
    if [[ ! "$confirm" =~ ^[jJyY]$ ]]; then
        echo "Abgebrochen."
        exit 0
    fi
    echo ""
fi

# Umbenennen
success=0
current_num=$next_num

for file in "${mp3_files[@]}"; do
    if [ $current_num -gt 100 ]; then
        echo -e "${RED}Maximum erreicht (100 Dateien)${NC}"
        break
    fi

    dst=$(printf "%03d.mp3" "$current_num")

    if [ -f "$file" ]; then
        mv "$file" "$dst"
        echo -e "${GREEN}✓${NC} $file → $dst"
        ((success++))
        ((current_num++))
    fi
done

echo ""
echo "========================================"
echo -e "${GREEN}Ergebnis: $success Dateien umbenannt${NC}"
echo "========================================"
echo ""
echo "Vorhandene MP3-Dateien:"
ls -lh *.mp3 2>/dev/null | head -15

# Hinweis für Pi-Upload
if [ $success -gt 0 ]; then
    echo ""
    echo -e "${CYAN}Nächster Schritt - auf Raspberry Pi kopieren:${NC}"
    echo "  scp 0*.mp3 pi@rover.local:~/selection-panel/media/"
fi
