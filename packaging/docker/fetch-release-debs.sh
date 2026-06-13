#!/bin/sh
# Download both per-arch gtk4-brotway .debs for a release tag into <outdir> (default
# ./debs). On a tag push image.yml races release.yml, so retry until both arches land.
# Needs gh + GITHUB_REPOSITORY (and a token in the env). Usage: fetch-release-debs.sh <tag> [outdir]
set -eu
TAG="${1:?usage: fetch-release-debs.sh <tag> [outdir]}"
OUT="${2:-debs}"
for i in $(seq 1 30); do
  rm -rf "$OUT"; mkdir -p "$OUT"
  gh release download "$TAG" --repo "$GITHUB_REPOSITORY" \
    --pattern 'gtk4-brotway_*.deb' --dir "$OUT" 2>/dev/null || true
  n="$(ls "$OUT"/gtk4-brotway_*.deb 2>/dev/null | wc -l)"
  if [ "$n" -ge 2 ]; then echo "got $n .debs"; break; fi
  echo "release assets not ready ($n found, attempt $i/30), waiting..."
  sleep 20
done
ls -l "$OUT"/*.deb
