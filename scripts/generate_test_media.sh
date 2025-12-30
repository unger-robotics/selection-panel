#!/bin/bash
#
# rename_media_1based.sh - Benennt Medien von 0-basiert zu 1-basiert um
#
# 000.jpg -> 001.jpg
# 001.jpg -> 002.jpg
# ...
# 009.jpg -> 010.jpg
#

set -e

MEDIA_DIR="${1:-./media}"

echo "=============================================="
echo "Medien umbenennen: 0-basiert -> 1-basiert"
echo "Verzeichnis: $MEDIA_DIR"
echo "=============================================="

if [[ ! -d "$MEDIA_DIR" ]]; then
    echo "Fehler: Verzeichnis existiert nicht: $MEDIA_DIR"
    exit 1
fi

# Prüfe ob 0-basierte Dateien existieren
if [[ ! -f "$MEDIA_DIR/000.jpg" ]]; then
    echo "Keine 0-basierten Dateien gefunden (000.jpg fehlt)"
    exit 1
fi

# Zähle Dateien
count=0
for f in "$MEDIA_DIR"/[0-9][0-9][0-9].jpg; do
    [[ -f "$f" ]] && ((count++))
done

echo "Gefunden: $count Medien-Paare"
echo ""

# Temporäres Verzeichnis
tmp_dir="$MEDIA_DIR/.rename_tmp"
mkdir -p "$tmp_dir"

# Phase 1: In temp mit neuem Namen (rückwärts um Konflikte zu vermeiden)
for i in $(seq $((count - 1)) -1 0); do
    old_id=$(printf "%03d" $i)
    new_id=$(printf "%03d" $((i + 1)))

    for ext in jpg mp3; do
        old_file="$MEDIA_DIR/$old_id.$ext"
        new_file="$tmp_dir/$new_id.$ext"

        if [[ -f "$old_file" ]]; then
            mv "$old_file" "$new_file"
            echo "  $old_id.$ext -> $new_id.$ext"
        fi
    done
done

# Phase 2: Zurück ins Hauptverzeichnis
mv "$tmp_dir"/* "$MEDIA_DIR/"
rmdir "$tmp_dir"

echo ""
echo "Fertig! $count Paare umbenannt."
echo "Medien jetzt: 001.jpg - $(printf "%03d" $count).jpg"
echo "=============================================="
