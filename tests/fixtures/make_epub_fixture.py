#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Regenerate tests/fixtures/book.epub (committed; stdlib only).

Spine:
  cover.xhtml   one full-page 600x900 PNG, no text      -> raster page
  ch1.xhtml     long text, several pages               -> vector pages
  ch2.xhtml     heading, 400x300 PNG figure and text   -> mixed page
nav.xhtml gives a two-entry outline.
"""

import struct
import zipfile
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent


def png(w: int, h: int, seed: int) -> bytes:
    rows = bytearray()
    for y in range(h):
        rows.append(0)
        for x in range(w):
            # Flat bands: compresses to a few KiB.
            band = ((x // 40 + y // 40 + seed) % 4) * 40
            rows += bytes((60 + band, 120, 200 - band))
    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data
                + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
            + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


def xhtml(title: str, body: str) -> str:
    return ('<?xml version="1.0" encoding="utf-8"?>\n'
            '<html xmlns="http://www.w3.org/1999/xhtml"><head>'
            f'<title>{title}</title></head><body>{body}</body></html>\n')


PARA = ("Reflowable text is laid out once per page size and font; every tile "
        "of a page then renders from the same display list. ")

files = {
    "mimetype": "application/epub+zip",
    "META-INF/container.xml": (
        '<?xml version="1.0"?>\n<container version="1.0" '
        'xmlns="urn:oasis:names:tc:opendocument:xmlns:container"><rootfiles>'
        '<rootfile full-path="OEBPS/content.opf" '
        'media-type="application/oebps-package+xml"/></rootfiles></container>\n'),
    "OEBPS/content.opf": (
        '<?xml version="1.0" encoding="utf-8"?>\n'
        '<package xmlns="http://www.idpf.org/2007/opf" version="3.0" '
        'unique-identifier="id"><metadata xmlns:dc="http://purl.org/dc/elements/1.1/">'
        '<dc:identifier id="id">thumtoo-test-book</dc:identifier>'
        '<dc:title>thumtoo test book</dc:title><dc:language>en</dc:language>'
        '</metadata><manifest>'
        '<item id="nav" href="nav.xhtml" media-type="application/xhtml+xml" properties="nav"/>'
        '<item id="cover" href="cover.xhtml" media-type="application/xhtml+xml"/>'
        '<item id="ch1" href="ch1.xhtml" media-type="application/xhtml+xml"/>'
        '<item id="ch2" href="ch2.xhtml" media-type="application/xhtml+xml"/>'
        '<item id="coverimg" href="cover.png" media-type="image/png"/>'
        '<item id="fig" href="fig.png" media-type="image/png"/>'
        '</manifest><spine><itemref idref="cover"/><itemref idref="ch1"/>'
        '<itemref idref="ch2"/></spine></package>\n'),
    "OEBPS/nav.xhtml": xhtml("Contents",
        '<nav xmlns:epub="http://www.idpf.org/2007/ops" epub:type="toc"><ol>'
        '<li><a href="ch1.xhtml">Chapter One</a></li>'
        '<li><a href="ch2.xhtml">Chapter Two</a></li></ol></nav>'),
    "OEBPS/cover.xhtml": xhtml("Cover",
        '<div style="margin:0;padding:0"><img src="cover.png" alt="" '
        'style="width:100%;height:100%"/></div>'),
    "OEBPS/ch1.xhtml": xhtml("Chapter One",
        "<h1>Chapter One</h1>" + "".join(f"<p>{i}. {PARA * 4}</p>" for i in range(60))),
    "OEBPS/ch2.xhtml": xhtml("Chapter Two",
        '<h1>Chapter Two</h1><p>A figure follows.</p>'
        '<p><img src="fig.png" alt="figure" style="width:80%"/></p>'
        f"<p>{PARA * 3}</p>"),
}

out = HERE / "book.epub"
with zipfile.ZipFile(out, "w") as z:
    z.writestr(zipfile.ZipInfo("mimetype"), files.pop("mimetype"),
               compress_type=zipfile.ZIP_STORED)
    for name, text in files.items():
        z.writestr(name, text, compress_type=zipfile.ZIP_DEFLATED)
    z.writestr("OEBPS/cover.png", png(600, 900, 11), compress_type=zipfile.ZIP_STORED)
    z.writestr("OEBPS/fig.png", png(400, 300, 99), compress_type=zipfile.ZIP_STORED)
print(f"wrote {out} ({out.stat().st_size // 1024} KiB)")
