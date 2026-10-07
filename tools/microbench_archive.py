#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Archive extract microbench (zipfile + unzip CLI).

Usage:
  python3 tools/microbench_archive.py /path/to/archive.zip

Reports TOC, sequential extract-all, first/last/scattered members, and
unzip -l / unzip -p when available. For libarchive/RAR numbers use a
thumtoo build with sample RARs.
"""
from __future__ import annotations

import argparse
import statistics
import subprocess
import time
import zipfile
from pathlib import Path


def timed(fn, n=7, warm=1):
    for _ in range(warm):
        fn()
    ts = []
    for _ in range(n):
        t0 = time.perf_counter()
        fn()
        ts.append(time.perf_counter() - t0)
    return statistics.median(ts) * 1000.0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("archive", type=Path)
    args = ap.parse_args()
    path = args.archive
    if not path.is_file():
        raise SystemExit(f"not a file: {path}")

    with zipfile.ZipFile(path) as zf:
        names = zf.namelist()
    print(f"archive={path} members={len(names)}")

    def toc():
        with zipfile.ZipFile(path) as zf:
            zf.namelist()

    def all_seq():
        with zipfile.ZipFile(path) as zf:
            for n in zf.namelist():
                zf.read(n)

    def one(i):
        with zipfile.ZipFile(path) as zf:
            zf.read(names[i])

    def scattered():
        step = max(1, len(names) // 10)
        with zipfile.ZipFile(path) as zf:
            for i in range(0, len(names), step):
                zf.read(names[i])

    print(f"  TOC namelist: {timed(toc, 15):.2f} ms")
    print(f"  extract all sequential: {timed(all_seq, 3):.2f} ms")
    print(f"  extract first: {timed(lambda: one(0), 15):.2f} ms")
    print(f"  extract last: {timed(lambda: one(-1), 15):.2f} ms")
    print(f"  ~10 scattered: {timed(scattered, 5):.2f} ms")

    try:
        def unzip_l():
            subprocess.check_output(["unzip", "-l", str(path)], stderr=subprocess.DEVNULL)

        def unzip_p():
            subprocess.check_output(
                ["unzip", "-p", str(path), names[-1]], stderr=subprocess.DEVNULL
            )

        print(f"  unzip -l: {timed(unzip_l, 10):.2f} ms")
        print(f"  unzip -p last: {timed(unzip_p, 10):.2f} ms")
    except FileNotFoundError:
        print("  unzip: not installed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
