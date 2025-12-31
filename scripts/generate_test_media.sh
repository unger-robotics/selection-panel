#!/bin/bash
#
# generate_test_media.sh - Generiert Test-Medien mit 1-basierter Nummerierung
#
# Erstellt 4K-Bilder mit der ID als Text und unterscheidbare Audio-Dateien.
# Verwendet Farben aus dem Projekt-Farbschema (farbschema.pdf)
#
# Verwendung:
#   ./generate_test_media.sh        # 10 Medien (001-010)
#   ./generate_test_media.sh 100    # 100 Medien (001-100)
#
# Benötigt: ImageMagick (convert), ffmpeg, bc
#

set -e

# Parameter
COUNT="${1:-10}"
MEDIA_DIR="${2:-./media}"

# 4K Auflösung
WIDTH=3840
HEIGHT=2160

# Farben aus dem Projekt-Farbschema (farbschema.pdf)
COLORS=(
    "#00979D"  # 001 Arduino Teal
    "#C51A4A"  # 002 Raspberry Red
    "#75A928"  # 003 Success Green
    "#D97706"  # 004 Warning Orange
    "#51CF66"  # 005 Signal Data (grün)
    "#FFD43B"  # 006 Signal Clock (gelb)
    "#4DABF7"  # 007 Signal Latch (blau)
    "#FF6B6B"  # 008 Signal VCC (rot)
    "#75264A"  # 009 Raspberry Purple
    "#005C5F"  # 010 Arduino Dark
)

COLOR_NAMES=(
    "Arduino Teal"
    "Raspberry Red"
    "Success Green"
    "Warning Orange"
    "Signal Data"
    "Signal Clock"
    "Signal Latch"
    "Signal VCC"
    "Raspberry Purple"
    "Arduino Dark"
)

# Musikalische Noten (Frequenzen in Hz)
# C4=262, D4=294, E4=330, F4=349, G4=392, A4=440, B4=494, C5=523, D5=587, E5=659
NOTES=(262 294 330 349 392 440 494 523 587 659)
NOTE_NAMES=("C4" "D4" "E4" "F4" "G4" "A4" "B4" "C5" "D5" "E5")

echo "=============================================="
echo "Test-Medien generieren (1-basiert, 4K)"
echo "=============================================="
echo "Anzahl:      $COUNT"
echo "Auflösung:   ${WIDTH}x${HEIGHT} (4K)"
echo "Verzeichnis: $MEDIA_DIR"
echo "Farbschema:  farbschema.pdf"
echo "=============================================="
echo ""

# Abhängigkeiten prüfen
for cmd in convert ffmpeg bc; do
    if ! command -v $cmd &> /dev/null; then
        echo "Fehler: $cmd nicht installiert."
        echo "Installation: sudo apt install imagemagick ffmpeg bc"
        exit 1
    fi
done

# Verzeichnis erstellen
mkdir -p "$MEDIA_DIR"

# Alte Medien löschen
echo "Lösche alte Medien..."
rm -f "$MEDIA_DIR"/*.jpg "$MEDIA_DIR"/*.mp3

echo ""
echo "Generiere $COUNT Medien-Paare..."
echo ""

# Medien generieren (1-basiert: 001 bis COUNT)
for i in $(seq 1 $COUNT); do
    id=$(printf "%03d" $i)
    
    # Farbe und Note aus Array (zyklisch für >10)
    idx=$(( (i - 1) % 10 ))
    color="${COLORS[$idx]}"
    color_name="${COLOR_NAMES[$idx]}"
    freq=${NOTES[$idx]}
    note_name=${NOTE_NAMES[$idx]}
    
    # ===== BILD: 4K mit farbigem Hintergrund und großer ID =====
    convert -size ${WIDTH}x${HEIGHT} xc:"$color" \
        -gravity center \
        -pointsize 600 \
        -fill white \
        -stroke "rgba(0,0,0,0.3)" -strokewidth 8 \
        -annotate 0 "$id" \
        -quality 90 \
        "$MEDIA_DIR/${id}.jpg"
    
    # ===== AUDIO: Verschiedene Sounds je nach ID =====
    output="$MEDIA_DIR/${id}.mp3"
    
    case $idx in
        0)  # 001: Sanfter Einzelton
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                -af "volume=0.7,afade=t=in:st=0:d=1,afade=t=out:st=13:d=2" \
                -y -loglevel error "$output"
            sound_desc="Sanfter Ton $note_name"
            ;;
        1)  # 002: Dur-Akkord (warm, fröhlich)
            f2=$(echo "$freq * 1.25" | bc)
            f3=$(echo "$freq * 1.5" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                   -f lavfi -i "sine=frequency=${f2}:duration=15" \
                   -f lavfi -i "sine=frequency=${f3}:duration=15" \
                   -filter_complex "[0][1][2]amix=inputs=3:duration=first,volume=0.6,afade=t=in:st=0:d=1,afade=t=out:st=13:d=2" \
                   -y -loglevel error "$output"
            sound_desc="Dur-Akkord $note_name"
            ;;
        2)  # 003: Sanftes Arpeggio (melodisch)
            f2=$(echo "$freq * 1.25" | bc)
            f3=$(echo "$freq * 1.5" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=1.5" \
                   -f lavfi -i "sine=frequency=${f2}:duration=1.5" \
                   -f lavfi -i "sine=frequency=${f3}:duration=2" \
                   -filter_complex "[0][1][2]concat=n=3:v=0:a=1,aloop=loop=2:size=220500,volume=0.7,afade=t=in:st=0:d=0.5,afade=t=out:st=13:d=2" \
                   -t 15 -y -loglevel error "$output"
            sound_desc="Arpeggio $note_name"
            ;;
        3)  # 004: Binaural Beat (meditativ, schwebend)
            f2=$(echo "$freq + 4" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                   -f lavfi -i "sine=frequency=${f2}:duration=15" \
                   -filter_complex "[0][1]amix=inputs=2:duration=first,volume=0.6,afade=t=in:st=0:d=2,afade=t=out:st=12:d=3" \
                   -y -loglevel error "$output"
            sound_desc="Binaural $note_name"
            ;;
        4)  # 005: Sanftes Wogen (langsam pulsierend wie Wellen)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                   -af "tremolo=f=0.5:d=0.4,volume=0.7,afade=t=in:st=0:d=1,afade=t=out:st=13:d=2" \
                   -y -loglevel error "$output"
            sound_desc="Wogen $note_name"
            ;;
        5)  # 006: Quinte (harmonisch, offen)
            f2=$(echo "$freq * 1.5" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                   -f lavfi -i "sine=frequency=${f2}:duration=15" \
                   -filter_complex "[0][1]amix=inputs=2:duration=first,volume=0.6,afade=t=in:st=0:d=1,afade=t=out:st=13:d=2" \
                   -y -loglevel error "$output"
            sound_desc="Quinte $note_name"
            ;;
        6)  # 007: Oktave (voll, rund)
            f2=$(echo "$freq * 2" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                   -f lavfi -i "sine=frequency=${f2}:duration=15" \
                   -filter_complex "[0][1]amix=inputs=2:duration=first,volume=0.6,afade=t=in:st=0:d=1,afade=t=out:st=13:d=2" \
                   -y -loglevel error "$output"
            sound_desc="Oktave $note_name"
            ;;
        7)  # 008: Glocke (Grundton + Obertöne, sanft ausklingend)
            f2=$(echo "$freq * 2" | bc)
            f3=$(echo "$freq * 3" | bc)
            f4=$(echo "$freq * 4" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                   -f lavfi -i "sine=frequency=${f2}:duration=12" \
                   -f lavfi -i "sine=frequency=${f3}:duration=8" \
                   -f lavfi -i "sine=frequency=${f4}:duration=5" \
                   -filter_complex "[0]volume=0.5[a];[1]volume=0.25,afade=t=out:st=10:d=2[b];[2]volume=0.15,afade=t=out:st=6:d=2[c];[3]volume=0.1,afade=t=out:st=3:d=2[d];[a][b][c][d]amix=inputs=4:duration=first,afade=t=in:st=0:d=0.5,afade=t=out:st=13:d=2" \
                   -y -loglevel error "$output"
            sound_desc="Glocke $note_name"
            ;;
        8)  # 009: Moll-Akkord (melancholisch, sanft)
            f2=$(echo "$freq * 1.2" | bc)
            f3=$(echo "$freq * 1.5" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=15" \
                   -f lavfi -i "sine=frequency=${f2}:duration=15" \
                   -f lavfi -i "sine=frequency=${f3}:duration=15" \
                   -filter_complex "[0][1][2]amix=inputs=3:duration=first,volume=0.6,afade=t=in:st=0:d=1,afade=t=out:st=13:d=2" \
                   -y -loglevel error "$output"
            sound_desc="Moll-Akkord $note_name"
            ;;
        9)  # 010: Dreiklang-Folge (I-IV-V, festlich aber sanft)
            f_iv=$(echo "$freq * 1.333" | bc)
            f_v=$(echo "$freq * 1.5" | bc)
            ffmpeg -f lavfi -i "sine=frequency=${freq}:duration=5" \
                   -f lavfi -i "sine=frequency=${f_iv}:duration=5" \
                   -f lavfi -i "sine=frequency=${f_v}:duration=5" \
                   -filter_complex "[0][1][2]concat=n=3:v=0:a=1,volume=0.7,afade=t=in:st=0:d=0.5,afade=t=out:st=13:d=2" \
                   -y -loglevel error "$output"
            sound_desc="Progression $note_name"
            ;;
    esac
    
    printf "  ✓ %s: %-17s | %s\n" "$id" "$color_name" "$sound_desc"
done

echo ""
echo "=============================================="
echo "Fertig! $COUNT Medien-Paare erstellt."
echo ""
echo "Auflösung: ${WIDTH}x${HEIGHT} (4K UHD)"
echo ""
echo "Übersicht:"
echo "  ID  | Farbe             | Sound"
echo "  ----|-------------------|--------------------"
echo "  001 | Arduino Teal      | C4 Sanfter Ton"
echo "  002 | Raspberry Red     | D4 Dur-Akkord"
echo "  003 | Success Green     | E4 Arpeggio"
echo "  004 | Warning Orange    | F4 Binaural (schwebend)"
echo "  005 | Signal Data       | G4 Wogen (langsam)"
echo "  006 | Signal Clock      | A4 Quinte"
echo "  007 | Signal Latch      | B4 Oktave"
echo "  008 | Signal VCC        | C5 Glocke"
echo "  009 | Raspberry Purple  | D5 Moll-Akkord"
echo "  010 | Arduino Dark      | E5 Progression"
echo ""
echo "Browser mit Cmd+Shift+R neu laden!"
echo "=============================================="

# Dateigröße anzeigen
echo ""
echo "Dateigrößen:"
ls -lh "$MEDIA_DIR"/*.jpg 2>/dev/null | awk '{print "  " $NF ": " $5}' | head -5
echo "  ..."
echo ""
du -sh "$MEDIA_DIR" 2>/dev/null | awk '{print "Gesamt: " $1}'
