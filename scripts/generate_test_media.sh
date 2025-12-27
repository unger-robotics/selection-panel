#!/bin/bash
#
# generate_test_media.sh - Erstellt/Verwaltet Test-Medien
# Version: 2.2.3 | Stand: 2025-12-26
#
# Farbschema: Arduino Teal + Raspberry Pi Red
# Konsistent mit: farbschema.tex, praesentation.tex, schematic.css,
#                 styles.css, create_amr_placeholders.sh
#
# Verwendung:
#   ./generate_test_media.sh              # Erstellt 10 Medien (000-009)
#   ./generate_test_media.sh 100          # Erstellt 100 Medien (000-099)
#   ./generate_test_media.sh --rename     # Benennt 001-010 -> 000-009 um
#   ./generate_test_media.sh --check      # Prüft vorhandene Medien
#   ./generate_test_media.sh --force 10   # Überschreibt existierende
#
# ID-Schema:
#   Button 0  -> 000.jpg, 000.mp3
#   Button 99 -> 099.jpg, 099.mp3
#

set -e

# Farben
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Standard-Werte
NUM_MEDIA=10
MEDIA_DIR="./media"
FORCE=false
ACTION="generate"

# =============================================================================
# Argument-Parsing
# =============================================================================

while [[ $# -gt 0 ]]; do
    case $1 in
        --rename)
            ACTION="rename"
            shift
            ;;
        --check)
            ACTION="check"
            shift
            ;;
        --force)
            FORCE=true
            shift
            ;;
        --dir)
            MEDIA_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Verwendung: $0 [OPTIONEN] [ANZAHL]"
            echo ""
            echo "Optionen:"
            echo "  --rename    Benennt 001-010 in 000-009 um"
            echo "  --check     Prüft vorhandene Medien"
            echo "  --force     Überschreibt existierende Dateien"
            echo "  --dir PATH  Zielverzeichnis (Standard: ./media)"
            echo ""
            echo "Beispiele:"
            echo "  $0              # 10 Medien erstellen"
            echo "  $0 100          # 100 Medien erstellen"
            echo "  $0 --rename     # 001-010 -> 000-009"
            exit 0
            ;;
        [0-9]*)
            NUM_MEDIA=$1
            shift
            ;;
        *)
            echo -e "${RED}Unbekannte Option: $1${NC}"
            exit 1
            ;;
    esac
done

# =============================================================================
# Funktionen
# =============================================================================

check_dependencies() {
    local missing=()
    
    if ! command -v convert &> /dev/null; then
        missing+=("ImageMagick")
        USE_IMAGEMAGICK=false
    else
        USE_IMAGEMAGICK=true
    fi
    
    if ! command -v ffmpeg &> /dev/null; then
        missing+=("ffmpeg")
        USE_FFMPEG=false
    else
        USE_FFMPEG=true
    fi
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        echo -e "${YELLOW}Fehlende Tools: ${missing[*]}${NC}"
        
        # Plattform-spezifische Hinweise
        if [[ "$OSTYPE" == "darwin"* ]]; then
            echo "  Mac: brew install imagemagick ffmpeg"
        else
            echo "  Linux: sudo apt install imagemagick ffmpeg"
        fi
        echo ""
    fi
}

# Prüft ob Medien vollständig sind
check_media() {
    echo "=============================================="
    echo "Medien-Check: $MEDIA_DIR"
    echo "=============================================="
    
    if [[ ! -d "$MEDIA_DIR" ]]; then
        echo -e "${RED}Verzeichnis existiert nicht: $MEDIA_DIR${NC}"
        exit 1
    fi
    
    local missing_jpg=0
    local missing_mp3=0
    local empty_files=0
    local ok_count=0
    
    for i in $(seq 0 $((NUM_MEDIA - 1))); do
        local id=$(printf "%03d" $i)
        local jpg="$MEDIA_DIR/$id.jpg"
        local mp3="$MEDIA_DIR/$id.mp3"
        local status="OK"
        
        if [[ ! -f "$jpg" ]]; then
            ((missing_jpg++))
            status="JPG fehlt"
        elif [[ ! -s "$jpg" ]]; then
            ((empty_files++))
            status="JPG leer"
        fi
        
        if [[ ! -f "$mp3" ]]; then
            ((missing_mp3++))
            status="MP3 fehlt"
        elif [[ ! -s "$mp3" ]]; then
            ((empty_files++))
            status="MP3 leer"
        fi
        
        if [[ "$status" == "OK" ]]; then
            ((ok_count++))
        else
            echo -e "  ${YELLOW}$id: $status${NC}"
        fi
    done
    
    echo ""
    echo "Zusammenfassung:"
    echo -e "  Vollständig: ${GREEN}$ok_count${NC} / $NUM_MEDIA"
    [[ $missing_jpg -gt 0 ]] && echo -e "  Fehlende JPGs: ${RED}$missing_jpg${NC}"
    [[ $missing_mp3 -gt 0 ]] && echo -e "  Fehlende MP3s: ${RED}$missing_mp3${NC}"
    [[ $empty_files -gt 0 ]] && echo -e "  Leere Dateien: ${RED}$empty_files${NC}"
    
    # Prüfe auf falsch nummerierte Dateien (1-basiert)
    if [[ -f "$MEDIA_DIR/001.jpg" ]] && [[ ! -f "$MEDIA_DIR/000.jpg" ]]; then
        echo ""
        echo -e "${YELLOW}HINWEIS: Dateien scheinen 1-basiert (001-010) statt 0-basiert (000-009)${NC}"
        echo "         Nutze: $0 --rename"
    fi
    
    echo "=============================================="
}

# Benennt 1-basierte in 0-basierte Dateien um
rename_media() {
    echo "=============================================="
    echo "Umbenennen: 1-basiert -> 0-basiert"
    echo "=============================================="
    
    if [[ ! -d "$MEDIA_DIR" ]]; then
        echo -e "${RED}Verzeichnis existiert nicht: $MEDIA_DIR${NC}"
        exit 1
    fi
    
    # Prüfe ob 1-basierte Dateien existieren
    if [[ ! -f "$MEDIA_DIR/001.jpg" ]]; then
        echo -e "${YELLOW}Keine 1-basierten Dateien gefunden (001.jpg fehlt)${NC}"
        exit 1
    fi
    
    # Prüfe ob 0-basierte schon existieren
    if [[ -f "$MEDIA_DIR/000.jpg" ]] && [[ "$FORCE" != true ]]; then
        echo -e "${RED}000.jpg existiert bereits. Nutze --force zum Überschreiben${NC}"
        exit 1
    fi
    
    # Temporäres Verzeichnis für Umbenennung
    local tmp_dir="$MEDIA_DIR/.rename_tmp"
    mkdir -p "$tmp_dir"
    
    # Zähle vorhandene Dateien
    local count=0
    for f in "$MEDIA_DIR"/[0-9][0-9][0-9].jpg; do
        [[ -f "$f" ]] && ((count++))
    done
    
    echo "Gefunden: $count Medien-Paare"
    echo ""
    
    # Phase 1: In temp verschieben mit neuem Namen
    for i in $(seq 1 $count); do
        local old_id=$(printf "%03d" $i)
        local new_id=$(printf "%03d" $((i - 1)))
        
        for ext in jpg mp3; do
            local old_file="$MEDIA_DIR/$old_id.$ext"
            local new_file="$tmp_dir/$new_id.$ext"
            
            if [[ -f "$old_file" ]]; then
                mv "$old_file" "$new_file"
                echo "  $old_id.$ext -> $new_id.$ext"
            fi
        done
    done
    
    # Phase 2: Aus temp zurück verschieben
    mv "$tmp_dir"/* "$MEDIA_DIR/"
    rmdir "$tmp_dir"
    
    echo ""
    echo -e "${GREEN}Fertig! $count Paare umbenannt.${NC}"
    echo "=============================================="
}

# Generiert Test-Medien
generate_media() {
    echo "=============================================="
    echo "Test-Medien Generator v2.2.3"
    echo "=============================================="
    echo "Anzahl:  $NUM_MEDIA (IDs: 000-$(printf "%03d" $((NUM_MEDIA - 1))))"
    echo "Ziel:    $MEDIA_DIR"
    echo "Force:   $FORCE"
    echo "=============================================="
    echo ""
    
    check_dependencies
    
    mkdir -p "$MEDIA_DIR"
    
    # ==========================================================================
    # FARBPALETTE: Arduino Teal + Raspberry Pi Red
    # Konsistent mit farbschema.tex
    # ==========================================================================
    local colors=(
        "#00979D"   # Arduino Teal (Primär)
        "#C51A4A"   # Raspberry Red
        "#75A928"   # Success Green
        "#005C5F"   # Arduino Dark
        "#75264A"   # Raspberry Purple
        "#D97706"   # Warning Orange
        "#62AEB2"   # Arduino Teal Light
        "#2D3A45"   # Slate Medium
        "#1E2933"   # Slate Dark
        "#0D1117"   # GitHub Dark
    )
    local color_names=(
        "ArduinoTeal"
        "RaspberryRed"
        "SuccessGreen"
        "ArduinoDark"
        "RaspberryPurple"
        "WarningOrange"
        "TealLight"
        "SlateMedium"
        "SlateDark"
        "GitHubDark"
    )
    
    local created_jpg=0
    local created_mp3=0
    local skipped=0
    
    for i in $(seq 0 $((NUM_MEDIA - 1))); do
        local id=$(printf "%03d" $i)
        local color_idx=$((i % ${#colors[@]}))
        local color="${colors[$color_idx]}"
        local color_name="${color_names[$color_idx]}"
        
        # JPG erstellen
        local jpg_file="$MEDIA_DIR/$id.jpg"
        if [[ ! -f "$jpg_file" ]] || [[ "$FORCE" == true ]]; then
            if [[ "$USE_IMAGEMAGICK" == true ]]; then
                # Textfarbe: weiß für dunkle Hintergründe, dunkel für helle
                local text_color="white"
                if [[ "$color" == "#62AEB2" ]] || [[ "$color" == "#75A928" ]] || [[ "$color" == "#D97706" ]]; then
                    text_color="#0D1117"
                fi
                
                convert -size 640x480 xc:"$color" \
                    -gravity center \
                    -pointsize 120 \
                    -fill "$text_color" \
                    -annotate 0 "$id" \
                    "$jpg_file" 2>/dev/null
                echo -e "  ${GREEN}[OK]${NC} $id.jpg ($color_name)"
                ((created_jpg++))
            fi
        else
            ((skipped++))
        fi
        
        # MP3 erstellen
        local mp3_file="$MEDIA_DIR/$id.mp3"
        if [[ ! -f "$mp3_file" ]] || [[ "$FORCE" == true ]]; then
            if [[ "$USE_FFMPEG" == true ]]; then
                local freq=$((220 + i * 20))
                ffmpeg -y -f lavfi -i "sine=frequency=$freq:duration=1" \
                    -acodec libmp3lame -b:a 128k \
                    "$mp3_file" 2>/dev/null
                echo -e "  ${GREEN}[OK]${NC} $id.mp3 (${freq}Hz)"
                ((created_mp3++))
            fi
        fi
    done
    
    echo ""
    echo "=============================================="
    echo "Zusammenfassung:"
    echo "  Erstellt: $created_jpg JPGs, $created_mp3 MP3s"
    [[ $skipped -gt 0 ]] && echo "  Übersprungen: $skipped (existieren bereits)"
    echo ""
    echo "Farbschema (konsistent mit farbschema.tex):"
    echo "  #00979D ArduinoTeal    #C51A4A RaspberryRed"
    echo "  #75A928 SuccessGreen   #D97706 WarningOrange"
    echo "  #005C5F ArduinoDark    #75264A RaspberryPurple"
    echo "=============================================="
}

# =============================================================================
# Hauptprogramm
# =============================================================================

case $ACTION in
    check)
        check_media
        ;;
    rename)
        rename_media
        ;;
    generate)
        generate_media
        ;;
esac
