<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Agent notes — thumtoo

## Intent

Latest agent handoff: **TODO.md** (live tip). Benchmark plan: [docs/BENCHMARK_KIT.md](docs/BENCHMARK_KIT.md).
thumtoo is a **cache and index library**, not a viewer. Do not add GUI code
here. Product rules for biltoo modes stay in biltoo’s DOMAIN/IDENTITY docs.
Post-release: prefer SQLite schema migration over cache wipe (see docs/DIRTOO_BACKBONE.md).
**Pixel durability / archives:** [docs/PIXEL_AND_ARCHIVE_POLICY.md](docs/PIXEL_AND_ARCHIVE_POLICY.md) (tiles+LQIP durable; soft ephemeral; Random vs Sequential archives).
**LQIP:** never generate standalone — only opportunistic free-data encode during tiles/soft ([docs/PIXEL_AND_ARCHIVE_POLICY.md](docs/PIXEL_AND_ARCHIVE_POLICY.md) §1.1).

## Related repositories

- biltoo: <https://github.com/Grumbel/biltoo>
- dirtoo: <https://github.com/Grumbel/dirtoo>
- galapix: <https://github.com/Galapix/galapix>
- dirtoo-py: <https://github.com/Grumbel/dirtoo-py>

## Documentation map

| File | Purpose |
|------|---------|
| [README.md](README.md) | What / build / scope table |
| [DESIGN.md](DESIGN.md) | Normative design + Phase 0 constants |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Tree and dependencies |
| [TODO.md](TODO.md) | Phase tracking |
| [INTEGRATION.md](INTEGRATION.md) | biltoo API mapping |
| [TAGS.md](TAGS.md) | content-hash tags vs dirtoo TagStore |
| [docs/PLAN.md](docs/PLAN.md) | Final pre-implementation plan (phases, locks) |
| [docs/DATABASE.md](docs/DATABASE.md) | DB redesign schema (index/bulk/user; dual-path) |
| [docs/API_MIGRATION.md](docs/API_MIGRATION.md) | Dual-path Client → Store cutover notes |
| [docs/BLOB_AND_OVERLAY.md](docs/BLOB_AND_OVERLAY.md) | Pointer only → DATABASE.md |
| [docs/DIRTOO_BACKBONE.md](docs/DIRTOO_BACKBONE.md) | dirtoo virtual FS backbone; schema migrate policy; tags/sets JSON |
| [docs/BENCHMARK_KIT.md](docs/BENCHMARK_KIT.md) | Benchmark kit plan (codec/archive/golden paths/corpus flake) |

## Handoff

## Status (2026-09-10)

**Tip: thumtoo-136-fz-resolve-helper.** Flatten + POD resolve_hash_link_page. Next: host verify.
Full session notes: top of [TODO.md](TODO.md).
Prior: `//pdfimage:N` keep_obj fix; PDF dual backend; live PDF tiles rgb888.


**Phase 1–2 done:** Client, prepare/status CLIs, system SQLite, libvips+JXL ladder,
archive TOC + member extract, tags, PDF `//page:N`, biltoo INTEGRATION.md.

**Phase 4 tiles done (library side):** 256² JPEG pyramid, Client get/request_tile,
`prepare --tiles`, status. See [TILES.md](TILES.md).

**Performance (2026-09-07):** Multi-worker job queue (`Client::open` worker count /
`thumtoo-prepare --jobs`); parallel per-scale JPEG encode; 512 MiB extract cache;
same-archive coalesce for probe **and** tiles/ladder. Prepare `--stats` prints
`wall=` vs summed `cpu:` scopes. Tip bundle: **thumtoo-023** (session handoff 2026-09-07). (Location URI API).

Galapix uses thumtoo as a flake input (source `THUMTOO_DIR`); interactive
`request_tile` is single-scale. Location URI parse/format is in-tree; next gaps: http(s) fetch, content-id blob resolve, single-cell cut if still open.

### Threading contract (do not break)
* `get_*` — cache-only, GUI-safe reads
* `request_*` — enqueue; callbacks via `Executor` (not raw worker threads for GUI)
* Workers share one SQLite (WAL + busy_timeout); watch lock storms at high jobs

**Later:** cache eviction / `thumtoo-gc`; optional D-Bus; video animated preview.

Ladder blobs: `blobs.sqlite`. Tile blobs will share that file under `tile_blobs`.



## Delivery by environment

| Agent | Repo access | Delivery |
|-------|-------------|----------|
| **Claude Code** (local, in the human’s checkout) | Direct | Commit directly on the current branch. No bundles. Do not push unless asked. |
| **Grok Web** (remote sandbox) | None to the human’s tree/remotes | Sequential **git bundles** — see below |

### Bundle handovers (Grok Web only)

Applies only to sandboxed agents that lack access to the human’s repository.
Deliver work as sequential **git bundles** (`thumtoo-NNN-slug.bundle` from
`HEAD`), copy to a downloadable artifacts path when available, and state the
path in the chat reply.

```bash
git pull /path/to/thumtoo-NNN-slug.bundle HEAD
```

Always emit a bundle for tips the human must integrate — a hash alone is not
enough if the commit never left the sandbox.

Standing rules live in this file and DESIGN/docs; the human’s first message can
be a short task + tip pointer. Refresh AGENTS.md / TODO.md at session end.

## Agent sandbox notes

Mostly relevant to sandboxed agents (Grok Web); local agents run in the
human’s checkout with the Nix dev shell.

- Full `nix build` of consumers (biltoo) may fail until **this** tip is pulled.
- No assumption of local MuPDF beyond what the flake provides (Poppler is not used).
- `make -k` is not the Nix default; use `cmake --build . -- -k` to collect
  many compile errors in one derivation.
- Keep declarations that use a type **after** that type is defined
  (`PdfRaster` before `std::optional<PdfRaster> …` in headers).
- Helpers appended after `} // namespace thumtoo` are a recurring footgun.

## Commits

Author: Ingo Ruhnke \<grumbel@gmail.com\>  
Co-authored-by: trailer naming the **actual model** that did the work (not a
hard-coded name), e.g. `Co-authored-by: Claude Opus 5.5 <noreply@anthropic.com>`
or `Co-authored-by: Grok <grok@x.ai>`  
License: GPL-3.0-or-later, REUSE SPDX headers on new files
