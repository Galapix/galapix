#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Command-line behavior of the benchmark tools, end to end.

Runs the built binaries on tiny generated fixtures and checks what scripts
depend on: exit codes (0 ok, 1 measurement failed, 2 usage), --help/--version,
CSV shape (one header, constant width, RFC 4180), JSON validity (single and
batch documents), that a damaged input is reported in-band without stopping the
batch, and that stdout carries only the selected format.

Usage: test_bench_tools_cli.py [--gp-archive BIN] [--gp-tile BIN]
                               [--microbench-decode BIN] [--thumtoo-bench BIN]
A tool whose binary is not given is skipped.
"""

import argparse
import base64
import csv
import io
import json
import os
import struct
import subprocess
import sys
import tarfile
import tempfile
import zipfile
import zlib

TINY_JPEG = base64.b64decode(
    "/9j/4AAQSkZJRgABAQAAAQABAAD/2wBDAAYEBQYFBAYGBQYHBwYIChAKCgkJChQODwwQFxQY"
    "GBcUFhYaHSUfGhsjHBYWICwgIyYnKSopGR8tMC0oMCUoKSj/2wBDAQcHBwoIChMKChMoGhYa"
    "KCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCgoKCj/wAAR"
    "CAAwAEADASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAA"
    "AgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkK"
    "FhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWG"
    "h4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl"
    "5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREA"
    "AgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYk"
    "NOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOE"
    "hYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk"
    "5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwD5qgs/ar8Fn7VpQWftWhDZ+1RGZnhcYZkN"
    "n7Vfhs/atOGz9qvwWftW0Zn0WFxhmwWftV+Cz9q0obP2q/DZ+1bRmfRYXGGbDZ+1X4bP2rTh"
    "tPar8Fn7VtGZ9DhcYebQ2ftV+Cz9q04LP2q/BZ+1eDGZ/OOFxhmwWftV+Gz9q0obP2rQhs/a"
    "tozPosLjDMhs/ar8Fn7VpwWftV+Gz9q2jM+hwuMMyCz9q0IbP2rShs/ar8Np7VtGZ9FhcYeb"
    "w2ftV+Gz9q0obP2rQgs/avBjM/nHC4wzILP2q/BZ+1acFn7Vfhs/atozPocLjDMhs/atCGz9"
    "q0obP2q/BZ+1bRmfRYXGGbBZ+1X4LP2rThs/ar8Nn7VtGZ9FhcYf/9k="
)

failures = []


def expect(cond, msg):
    if not cond:
        failures.append(msg)
        print("FAIL:", msg, file=sys.stderr)


def run(binary, *args, timeout=300):
    p = subprocess.run([binary, *map(str, args)], capture_output=True, text=True, timeout=timeout)
    return p.returncode, p.stdout, p.stderr


def parse_csv(text):
    rows = list(csv.reader(io.StringIO(text)))
    return (rows[0], rows[1:]) if rows else ([], [])


def write_png(path, w=96, h=64):
    """Minimal 8-bit RGB PNG with a gradient (no imaging library needed)."""
    raw = b"".join(
        b"\x00" + bytes(v for x in range(w) for v in (x * 255 // w, y * 255 // h, (x + y) % 256))
        for y in range(h)
    )

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw)))
        f.write(chunk(b"IEND", b""))


def common_checks(name, binary, needs_input_msg):
    """Behavior every tool shares."""
    rc, out, err = run(binary, "--help")
    expect(rc == 0 and out.startswith("Usage: "), f"{name}: --help exits 0 with usage on stdout")
    expect("Exit status:" in out and "Examples:" in out, f"{name}: --help has exit status + examples")
    expect(max((len(l) for l in out.splitlines()), default=0) <= 80, f"{name}: --help fits 80 columns")
    expect(err == "", f"{name}: --help prints nothing on stderr")

    rc, out, err = run(binary, "--version")
    expect(rc == 0 and "thumtoo" in out, f"{name}: --version")

    rc, out, err = run(binary)
    expect(rc == 2 and needs_input_msg in err and "--help" in err and out == "",
           f"{name}: no input is a usage error (exit 2, message on stderr)")

    rc, out, err = run(binary, "--bogus-option", "x")
    expect(rc == 2 and "unknown option '--bogus-option'" in err and out == "",
           f"{name}: unknown option is exit 2")

    rc, out, err = run(binary, "--repeat" if name != "thumtoo-bench" else "--jobs", "abc", "x")
    expect(rc == 2 and "invalid value 'abc'" in err, f"{name}: non-numeric value is exit 2")

    rc, out, err = run(binary, "--csv", "--json", "x")
    expect(rc == 2 and "cannot be combined" in err, f"{name}: --csv with --json is exit 2")

    rc, out, err = run(binary, "--no-header", "x")
    expect(rc == 2 and "--no-header" in err, f"{name}: --no-header without --csv is exit 2")

    rc, out, err = run(binary, "/nonexistent/definitely-missing")
    expect(rc == 2 and "cannot read" in err, f"{name}: missing path is exit 2")


def test_gp_archive(binary, tmp):
    zpath = os.path.join(tmp, "a.zip")
    with zipfile.ZipFile(zpath, "w", zipfile.ZIP_STORED) as z:
        for i in range(5):
            z.writestr(f"page{i}.bin", bytes([i]) * 20000)
    tpath = os.path.join(tmp, "b.tar")
    with tarfile.open(tpath, "w") as t:
        for i in range(5):
            data = bytes([i]) * 20000
            info = tarfile.TarInfo(f"page{i}.bin")
            info.size = len(data)
            t.addfile(info, io.BytesIO(data))
    bad = os.path.join(tmp, "damaged.zip")
    with open(zpath, "rb") as f:
        open(bad, "wb").write(f.read()[:60])
    d = os.path.join(tmp, "dir")
    os.makedirs(d)
    for src in (zpath, tpath):
        open(os.path.join(d, os.path.basename(src)), "wb").write(open(src, "rb").read())
    open(os.path.join(d, "notes.txt"), "w").write("not an archive")

    common_checks("gp-archive", binary, "missing ARCHIVE")
    rc, out, err = run(binary, "-b", "no-such-backend", zpath)
    expect(rc == 2 and "unknown backend" in err, "gp-archive: unknown backend is exit 2")
    rc, out, err = run(binary, "-n", "0", zpath)
    expect(rc == 2 and "--repeat" in err, "gp-archive: --repeat 0 is exit 2")
    rc, out, err = run(binary, "--repat", "3", zpath)
    expect(rc == 2 and "did you mean '--repeat'" in err, "gp-archive: typo suggestion")

    # Default: human readable, not CSV or JSON.
    rc, out, err = run(binary, "-n", "1", zpath)
    expect(rc == 0 and "TOC ms" in out and "KiB" in out and not out.lstrip().startswith(("{", "archive,")),
           "gp-archive: default output is a readable table")

    # CSV.
    rc, out, err = run(binary, "-n", "1", "--csv", zpath, tpath)
    header, rows = parse_csv(out)
    expect(rc == 0 and header[:5] == ["archive", "format", "backend", "status", "reason"]
           and header[-1] == "overall_ratio", "gp-archive: csv header")
    expect(len(rows) == 2 and all(len(r) == len(header) for r in rows), "gp-archive: csv one row per archive")
    expect(all(r[3] == "ok" for r in rows) and {r[1] for r in rows} == {"zip", "tar"}, "gp-archive: csv values")
    rc, out2, _ = run(binary, "-n", "1", "--csv", "--no-header", zpath)
    expect(rc == 0 and out2.count("\n") == 1 and not out2.startswith("archive,"), "gp-archive: --no-header")

    # Comparison (works with one backend too; then there is simply nothing to rank).
    rc, out, err = run(binary, "-n", "1", "-b", "all", "--csv", zpath)
    header, rows = parse_csv(out)
    expect(rc == 0 and rows and all(r[3] in ("ok", "unsupported") for r in rows), "gp-archive: --backend all csv")

    # JSON: single document and batch.
    rc, out, err = run(binary, "-n", "1", "--json", zpath)
    doc = json.loads(out)
    expect(rc == 0 and doc["schema"] == 1 and doc["status"] == "ok" and "metrics" in doc and doc["members"] == 5,
           "gp-archive: single json document")
    rc, out, err = run(binary, "-n", "1", "-b", "all", "--json", zpath, tpath)
    doc = json.loads(out)
    expect(rc == 0 and doc["kind"] == "batch" and len(doc["results"]) == 2 and "aggregate" in doc,
           "gp-archive: batch json with aggregate")

    # Directory: archives found by content, the text file ignored.
    rc, out, err = run(binary, "-n", "1", "--csv", d)
    header, rows = parse_csv(out)
    expect(rc == 0 and len(rows) == 2, "gp-archive: directory expands to the two archives")

    # A damaged archive is reported in-band, the rest still runs, exit 1.
    for fmt in ("--csv", "--json", None):
        args = ["-n", "1"] + ([fmt] if fmt else []) + [bad, zpath]
        rc, out, err = run(binary, *args)
        expect(rc == 1 and "could not be measured" in err, f"gp-archive: damaged archive exits 1 ({fmt or 'text'})")
        if fmt == "--csv":
            _, rows = parse_csv(out)
            expect([r[3] for r in rows] == ["failed", "ok"] and rows[0][4], "gp-archive: csv failed row has a reason")
        elif fmt == "--json":
            doc = json.loads(out)
            expect([r["status"] for r in doc["results"]] == ["failed", "ok"], "gp-archive: json failed result in-band")
        else:
            expect("damaged.zip" in out and "failed" in out and "a.zip" in out, "gp-archive: text reports failure and the rest")


def test_gp_tile(binary, tmp):
    png = os.path.join(tmp, "grad.png")
    write_png(png)
    d = os.path.join(tmp, "imgs")
    os.makedirs(d)
    open(os.path.join(d, "one.png"), "wb").write(open(png, "rb").read())
    open(os.path.join(d, "two.png"), "wb").write(open(png, "rb").read())
    open(os.path.join(d, "notes.txt"), "w").write("nope")
    notimg = os.path.join(tmp, "notimage.png")
    open(notimg, "w").write("this is not a png")

    common_checks("gp-tile", binary, "missing FILE")
    for args, needle in ((["-q", "80,abc"], "--quality"), (["-q", "0"], "--quality"), (["-t", "8"], "--tile"),
                         (["-c", "nope"], "unknown codec"), (["-c", "jpeg@e3"], "no effort"),
                         (["-c", "jpeg,webp", "--reference", "jxl:80"], "--reference"),
                         (["--reference", "jpeg"], "--reference")):
        rc, out, err = run(binary, *args, png)
        expect(rc == 2 and needle in err, f"gp-tile: {' '.join(args)} is exit 2 mentioning {needle}")

    rc, out, err = run(binary, "-q", "60,80", "-n", "1", png)
    expect(rc == 0 and "PSNR dB" in out and "KiB" in out, "gp-tile: default output is a readable table")

    rc, out, err = run(binary, "-q", "60,80", "-n", "1", "--csv", png)
    header, rows = parse_csv(out)
    expect(rc == 0 and header[0] == "file" and header[-2:] == ["matched", "overall_ratio"] and len(rows) == 2
           and all(len(r) == len(header) for r in rows), "gp-tile: csv shape (one row per quality)")

    rc, out, err = run(binary, "-q", "80", "-n", "1", "--json", png)
    doc = json.loads(out)
    expect(rc == 0 and doc["status"] == "ok" and doc["rows"][0]["codec"] == "jpeg", "gp-tile: single json")

    # Comparison; libvips may lack an encoder, so accept ok/unsupported per codec.
    rc, out, err = run(binary, "-c", "jpeg,jpeg", "-q", "80", "-n", "1", "--json", png)
    expect(rc == 0 and json.loads(out).get("kind") is None, "gp-tile: duplicate codecs collapse to one")
    rc, out, err = run(binary, "-c", "jpeg,webp", "-q", "60,90", "-n", "1", "--json", png)
    doc = json.loads(out)
    expect(doc["kind"] == "compare" and [c["codec"] for c in doc["codecs"]] == ["jpeg", "webp"],
           "gp-tile: compare json")
    rc, out, err = run(binary, "-c", "jpeg,webp", "-q", "60,90", "-n", "1", "--csv", png)
    header, rows = parse_csv(out)
    expect(all(len(r) == len(header) for r in rows) and {r[3] for r in rows} == {"jpeg", "webp"},
           "gp-tile: compare csv covers both codecs")

    # Directory and batch.
    rc, out, err = run(binary, "-q", "80", "-n", "1", "--csv", d)
    header, rows = parse_csv(out)
    expect(rc == 0 and len(rows) == 2, "gp-tile: directory expands to the two images")
    rc, out, err = run(binary, "-q", "80", "-n", "1", "--json", d)
    doc = json.loads(out)
    expect(doc["kind"] == "batch" and len(doc["results"]) == 2, "gp-tile: batch json")

    # An unreadable image is reported in-band and the rest runs.
    for fmt in ("--csv", "--json", None):
        args = ["-q", "80", "-n", "1"] + ([fmt] if fmt else []) + [notimg, png]
        rc, out, err = run(binary, *args)
        expect(rc == 1 and "could not be measured" in err, f"gp-tile: unreadable image exits 1 ({fmt or 'text'})")
        if fmt == "--csv":
            _, rows = parse_csv(out)
            expect([r[6] for r in rows] == ["failed", "ok"], "gp-tile: csv failed row")
        elif fmt == "--json":
            expect([r["status"] for r in json.loads(out)["results"]] == ["failed", "ok"], "gp-tile: json failed in-band")


def test_microbench(binary, tmp):
    jpg = os.path.join(tmp, "tiny.jpg")
    open(jpg, "wb").write(TINY_JPEG)
    png = os.path.join(tmp, "grad.png")
    write_png(png)
    cut = os.path.join(tmp, "cut.jpg")
    open(cut, "wb").write(TINY_JPEG[:300])
    d = os.path.join(tmp, "jpgs")
    os.makedirs(d)
    open(os.path.join(d, "a.jpg"), "wb").write(TINY_JPEG)
    open(os.path.join(d, "b.jpg"), "wb").write(TINY_JPEG)
    write_png(os.path.join(d, "c.png"))

    common_checks("microbench-decode", binary, "missing FILE")

    rc, out, err = run(binary, "-n", "1", jpg)
    expect(rc == 0 and "shrink2" in out and "Times in milliseconds" in out, "microbench: default output is a table")

    rc, out, err = run(binary, "-n", "1", "--csv", jpg, d)
    header, rows = parse_csv(out)
    expect(rc == 0 and header == ["file", "width", "height", "mpix", "status", "reason", "size_ms", "full_ms",
                                  "shrink2_ms", "shrink4_ms", "shrink8_ms", "thumb32_ms", "thumb256_ms"]
           and len(rows) == 3 and all(r[4] == "ok" for r in rows), "microbench: csv columns; directory finds the JPEGs only")

    rc, out, err = run(binary, "-n", "1", "--json", jpg)
    doc = json.loads(out)
    case = doc["cases"][0]
    expect(doc["schema"] == 1 and doc["warmups"] == 1 and case["file"] == "tiny.jpg" and case["width"] == 64
           and set(case["metrics"]) == {"size_ms", "full_ms", "shrink2_ms", "shrink4_ms", "shrink8_ms",
                                        "thumb32_ms", "thumb256_ms"}, "microbench: json keeps the baseline shape")

    # Non-JPEG and truncated JPEG: failed, never a bogus near-zero timing.
    for bad, why in ((png, "not a JPEG"), (cut, "truncated")):
        rc, out, err = run(binary, "-n", "1", "--json", bad, jpg)
        doc = json.loads(out)
        expect(rc == 1 and [c["status"] for c in doc["cases"]] == ["failed", "ok"] and "metrics" not in doc["cases"][0],
               f"microbench: {why} file is failed in-band, exit 1")
    rc, out, err = run(binary, "-n", "1", png, jpg)
    expect(rc == 1 and "png: failed" in out and "tiny.jpg" in out, "microbench: text lists the failure")


def test_thumtoo_bench(binary, tmp):
    jpg = os.path.join(tmp, "tiny.jpg")
    open(jpg, "wb").write(TINY_JPEG)
    cache = os.path.join(tmp, "bench-cache")

    common_checks("thumtoo-bench", binary, "missing PATH")
    for args, needle in ((["--cell", "0,1"], "--cell"), (["--ladder", "-5"], "--ladder"),
                         (["--min-scale", "5", "--max-scale", "2"], "--min-scale")):
        rc, out, err = run(binary, "--cache", cache, *args, jpg)
        expect(rc == 2 and needle in err, f"thumtoo-bench: {' '.join(args)} is exit 2")

    rc, out, err = run(binary, "--cache", cache, "--jobs", "2", jpg)
    expect(rc == 0 and "phase" in out and "total" in out, "thumtoo-bench: default output has a phase table")

    rc, out, err = run(binary, "--cache", cache, "--jobs", "2", "--json", jpg)
    doc = json.loads(out)
    expect(rc == 0 and doc["tool"] == "thumtoo-bench" and doc["phases"][0]["name"] == "probe"
           and "counters" in doc["phases"][0] and "stats" in doc["phases"][0], "thumtoo-bench: json keeps keys, adds counters")

    rc, out, err = run(binary, "--cache", cache, "--jobs", "2", "--csv", jpg)
    header, rows = parse_csv(out)
    expect(rc == 0 and header[0] == "phase" and rows[-1][0] == "total" and all(len(r) == len(header) for r in rows),
           "thumtoo-bench: csv has a total row")

    # An empty directory holds nothing to measure whatever the library accepts.
    empty = os.path.join(tmp, "empty")
    os.makedirs(empty)
    rc, out, err = run(binary, "--cache", cache, empty)
    expect(rc == 1 and "no readable image" in err, "thumtoo-bench: nothing to measure is exit 1")


def main():
    ap = argparse.ArgumentParser()
    for opt in ("gp-archive", "gp-tile", "microbench-decode", "thumtoo-bench"):
        ap.add_argument("--" + opt)
    args = ap.parse_args()
    tests = (("gp-archive", test_gp_archive), ("gp-tile", test_gp_tile),
             ("microbench-decode", test_microbench), ("thumtoo-bench", test_thumtoo_bench))
    ran = []
    with tempfile.TemporaryDirectory() as tmp:
        for name, fn in tests:
            binary = getattr(args, name.replace("-", "_"))
            if not binary:
                print(f"skip: {name} (no binary given)")
                continue
            sub = os.path.join(tmp, name)
            os.makedirs(sub)
            try:
                fn(binary, sub)
            except Exception as e:  # a crash in one tool must not hide the others
                expect(False, f"{name}: test aborted: {type(e).__name__}: {e}")
            ran.append(name)
    if failures:
        print(f"{len(failures)} failure(s)", file=sys.stderr)
        return 1
    print("ok: bench tools cli (" + ", ".join(ran) + ")")
    return 0


if __name__ == "__main__":
    sys.exit(main())
