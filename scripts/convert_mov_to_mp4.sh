#!/usr/bin/env bash
set -euo pipefail

# Batch-Konvertierung: *.MOV/*.mov -> *.mp4 (H.264 + AAC, kompatibel)
# Usage:
#   ./scripts/convert_mov_to_mp4.sh
#   ./scripts/convert_mov_to_mp4.sh assets/video
#   ./scripts/convert_mov_to_mp4.sh -f assets/video
#
# Optional:
#   CRF=20 PRESET=slow ./scripts/convert_mov_to_mp4.sh assets/video

FORCE=0
if [[ "${1:-}" == "-f" || "${1:-}" == "--force" ]]; then
  FORCE=1
  shift
fi

INPUT_DIR="${1:-assets/video}"
CRF="${CRF:-20}"          # 18..23: kleiner = bessere Qualität
PRESET="${PRESET:-slow}"  # ultrafast..veryslow

need() { command -v "$1" >/dev/null 2>&1 || { echo "Fehlt: $1"; exit 1; }; }
need ffmpeg
need ffprobe

if [[ ! -d "$INPUT_DIR" ]]; then
  echo "Ordner nicht gefunden: $INPUT_DIR"
  exit 1
fi

converted=0
skipped=0

while IFS= read -r -d '' in; do
  out="${in%.*}.mp4"

  if [[ -f "$out" && "$FORCE" -eq 0 ]]; then
    echo "SKIP: $out existiert"
    ((skipped++))
    continue
  fi

  tmp="${out}.tmp.$$.mp4"
  rm -f "$tmp"

  echo "CONVERT: $in -> $out"

  # Prüfe, ob es einen decodierbaren Audio-Stream gibt (Codec != "none")
  acodec="$(ffprobe -v error -select_streams a:0 \
    -show_entries stream=codec_name -of default=nk=1:nw=1 \
    "$in" 2>/dev/null || true)"

  audio_map=()
  audio_opts=()

  if [[ -n "$acodec" && "$acodec" != "none" ]]; then
    audio_map=(-map 0:a:0)
    audio_opts=(-c:a aac -b:a 192k)
  else
    audio_opts=(-an)
  fi

  ffmpeg -hide_banner -loglevel error -stats \
    -i "$in" \
    -map 0:v:0 "${audio_map[@]}" \
    -c:v libx264 -pix_fmt yuv420p -preset "$PRESET" -crf "$CRF" \
    "${audio_opts[@]}" \
    -sn -dn \
    -movflags +faststart \
    -f mp4 \
    "$tmp"

  mv -f "$tmp" "$out"
  ((converted++))
done < <(find "$INPUT_DIR" -type f -iname '*.mov' -print0)

echo "Fertig: ${converted} konvertiert, ${skipped} übersprungen."
