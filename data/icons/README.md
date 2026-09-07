# Tool icons

**Source of truth:** `svg/*.svg` (colored 24×24 glyphs).

**PNG at build time:** CMake runs `rsvg-convert` (preferred) or ImageMagick
`convert` and writes:

```
${CMAKE_BINARY_DIR}/share/galapix/icons/hicolor/24x24/actions/*.png
```

ImGui loads those PNGs via SDL_image (no SVG at runtime).

`GALAPIX_DATADIR` overrides the data root; otherwise the build-tree path from
`GALAPIX_DEFAULT_DATADIR` or `$prefix/share/galapix` is used.

Do not commit generated PNGs.
