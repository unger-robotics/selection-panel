#!/bin/bash
# ==============================================================================
# rename_images.sh - Benennt heruntergeladene Bilder für Selection Panel
# ==============================================================================
#
# Verwendung:
#   ./rename_images.sh [VERZEICHNIS] [--auto]
#
# Erkennt automatisch JPG/JPEG/PNG-Dateien und benennt sie zu 001.jpg - 010.jpg
# Konvertiert PNG automatisch zu JPG (falls ImageMagick installiert)
#
# Optionen:
#   --auto, -a    Keine Bestätigung erfragen
#   --resize, -r  Bilder auf 1920x1080 skalieren (spart Speicher)
#
# ==============================================================================

set -e

MEDIA_DIR="."
AUTO_MODE=false
RESIZE_MODE=false
TARGET_WIDTH=1920
TARGET_HEIGHT=1080

# Parameter parsen
for arg in "$@"; do
    case $arg in
        --auto|-a)
            AUTO_MODE=true
            ;;
        --resize|-r)
            RESIZE_MODE=true
            ;;
        *)
            if [[ ! "$arg" =~ ^- ]]; then
                MEDIA_DIR="$arg"
            fi
            ;;
    esac
done

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "========================================"
echo "Selection Panel - Image Rename Script"
echo "========================================"
echo ""

cd "$MEDIA_DIR" || {
    echo -e "${RED}Fehler: Verzeichnis '$MEDIA_DIR' nicht gefunden${NC}"
    exit 1
}

echo "Arbeitsverzeichnis: $(pwd)"
if [ "$RESIZE_MODE" = true ]; then
    echo -e "${CYAN}Resize-Modus: ${TARGET_WIDTH}x${TARGET_HEIGHT}${NC}"
fi
echo ""

# Prüfen ob ImageMagick verfügbar ist (für PNG-Konvertierung und Resize)
HAS_CONVERT=false
if command -v convert &> /dev/null; then
    HAS_CONVERT=true
fi

HAS_SIPS=false
if command -v sips &> /dev/null; then
    HAS_SIPS=true  # macOS built-in
fi

# Alle Bilddateien finden (ausser bereits umbenannte 001-099)
shopt -s nullglob nocaseglob
image_files=()
for f in *.jpg *.jpeg *.png *.webp; do
    # Überspringe bereits nummerierte Dateien (001.jpg bis 099.jpg)
    if [[ ! "$f" =~ ^0[0-9][0-9]\.(jpg|jpeg|png)$ ]]; then
        image_files+=("$f")
    fi
done
shopt -u nullglob nocaseglob

# Sortieren nach Name
IFS=$'\n' image_files=($(sort <<<"${image_files[*]}")); unset IFS

if [ ${#image_files[@]} -eq 0 ]; then
    echo -e "${YELLOW}Keine neuen Bilddateien gefunden.${NC}"
    echo ""
    echo "Vorhandene Dateien:"
    ls -lh *.jpg *.jpeg *.png 2>/dev/null || echo "(keine)"
    exit 0
fi

echo -e "${CYAN}Gefundene Bilddateien (${#image_files[@]}):${NC}"
for i in "${!image_files[@]}"; do
    num=$((i+1))
    # Dateigröße anzeigen
    size=$(ls -lh "${image_files[$i]}" 2>/dev/null | awk '{print $5}')
    echo "  $(printf '%2d' $num). ${image_files[$i]} ($size)"
done
echo ""

# Nächste freie Nummer finden
next_num=1
while [ -f "$(printf '%03d.jpg' $next_num)" ] && [ $next_num -le 100 ]; do
    ((next_num++))
done

echo -e "${CYAN}Starte bei Nummer: $(printf '%03d' $next_num)${NC}"
echo ""

# Prüfen ob genug Platz (max 100)
end_num=$((next_num + ${#image_files[@]} - 1))
if [ $end_num -gt 100 ]; then
    echo -e "${RED}Warnung: Nur $((100 - next_num + 1)) Plätze frei (001-100)${NC}"
fi

# Bestätigung
if [ "$AUTO_MODE" = false ]; then
    echo "Umbenennen zu $(printf '%03d' $next_num).jpg - $(printf '%03d' $end_num).jpg? (j/n)"
    read -r confirm
    if [[ ! "$confirm" =~ ^[jJyY]$ ]]; then
        echo "Abgebrochen."
        exit 0
    fi
    echo ""
fi

# Umbenennen und optional konvertieren/resizen
success=0
converted=0
resized=0
current_num=$next_num

for file in "${image_files[@]}"; do
    if [ $current_num -gt 100 ]; then
        echo -e "${RED}Maximum erreicht (100 Dateien)${NC}"
        break
    fi
    
    dst=$(printf "%03d.jpg" "$current_num")
    ext="${file##*.}"
    ext_lower=$(echo "$ext" | tr '[:upper:]' '[:lower:]')
    
    if [ -f "$file" ]; then
        # PNG/WEBP zu JPG konvertieren
        if [[ "$ext_lower" == "png" ]] || [[ "$ext_lower" == "webp" ]]; then
            if [ "$HAS_CONVERT" = true ]; then
                if [ "$RESIZE_MODE" = true ]; then
                    convert "$file" -resize "${TARGET_WIDTH}x${TARGET_HEIGHT}>" -quality 90 "$dst"
                    ((resized++))
                else
                    convert "$file" -quality 90 "$dst"
                fi
                rm "$file"
                echo -e "${GREEN}✓${NC} $file → $dst (konvertiert)"
                ((converted++))
            elif [ "$HAS_SIPS" = true ]; then
                # macOS: sips für Konvertierung
                sips -s format jpeg "$file" --out "$dst" &>/dev/null
                if [ "$RESIZE_MODE" = true ]; then
                    sips -Z $TARGET_WIDTH "$dst" &>/dev/null
                    ((resized++))
                fi
                rm "$file"
                echo -e "${GREEN}✓${NC} $file → $dst (konvertiert mit sips)"
                ((converted++))
            else
                echo -e "${YELLOW}⚠${NC} $file übersprungen (PNG, kein ImageMagick)"
                continue
            fi
        else
            # JPG/JPEG: nur umbenennen (optional resizen)
            if [ "$RESIZE_MODE" = true ] && [ "$HAS_CONVERT" = true ]; then
                convert "$file" -resize "${TARGET_WIDTH}x${TARGET_HEIGHT}>" -quality 90 "$dst"
                rm "$file"
                echo -e "${GREEN}✓${NC} $file → $dst (resized)"
                ((resized++))
            elif [ "$RESIZE_MODE" = true ] && [ "$HAS_SIPS" = true ]; then
                mv "$file" "$dst"
                sips -Z $TARGET_WIDTH "$dst" &>/dev/null
                echo -e "${GREEN}✓${NC} $file → $dst (resized mit sips)"
                ((resized++))
            else
                mv "$file" "$dst"
                echo -e "${GREEN}✓${NC} $file → $dst"
            fi
        fi
        ((success++))
        ((current_num++))
    fi
done

echo ""
echo "========================================"
echo -e "${GREEN}Ergebnis: $success Dateien verarbeitet${NC}"
if [ $converted -gt 0 ]; then
    echo -e "  PNG/WEBP konvertiert: $converted"
fi
if [ $resized -gt 0 ]; then
    echo -e "  Resized: $resized"
fi
echo "========================================"
echo ""
echo "Vorhandene JPG-Dateien:"
ls -lh *.jpg 2>/dev/null | head -15

# Hinweis für Pi-Upload
if [ $success -gt 0 ]; then
    echo ""
    echo -e "${CYAN}Nächster Schritt - auf Raspberry Pi kopieren:${NC}"
    echo "  scp 0*.jpg pi@rover.local:~/selection-panel/media/"
fi
