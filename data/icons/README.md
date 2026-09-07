# Tool icons

Canonical sources: `svg/*.svg` (colored 24×24 glyphs).

Raster for ImGui: `hicolor/24x24/actions/*.png` (RGBA, from ImageMagick).

```bash
for f in data/icons/svg/*.svg; do
  convert -background none -density 192 "$f" -resize 24x24 \
    PNG32:"data/icons/hicolor/24x24/actions/$(basename "$f" .svg).png"
done
```

Color roles (roughly):

| Icon | Hue |
|------|-----|
| pan | blue |
| zoom rect | amber / orange |
| grid tool | green |
| move | purple |
| grid / grid pin | cyan (+ red pin) |
| layouts | multi-color tiles |
