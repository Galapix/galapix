# Tool icons

Canonical sources: `svg/*.svg` (24×24, light strokes on transparent).

Raster for ImGui: `hicolor/24x24/actions/*.png` (generated with ImageMagick
`convert -background none … -resize 24x24`).

Regenerate PNGs after editing SVG:

```bash
for f in data/icons/svg/*.svg; do
  convert -background none -density 192 "$f" -resize 24x24 \
    "data/icons/hicolor/24x24/actions/$(basename "$f" .svg).png"
done
```

ImGui loads PNGs at runtime (`GALAPIX_DATADIR` or `./data`). Optional future:
build-time rsvg rule instead of committed PNGs.
