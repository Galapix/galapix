<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Host cutover — Store-only Client

**Audience:** biltoo (and other hosts) after thumtoo Phase E.

Library spine (0.1.0): see [PLAN.md](PLAN.md) success criteria and
[API_MIGRATION.md](API_MIGRATION.md). This document is the **host** checklist
for tile-native behaviour on the Store-only Client (≥262).

---

## 1. What hosts get (Store-only, ≥262)

| Open args | Layout |
|-----------|--------|
| `Client::open(cache_root, executor, workers, data_root)` | Store at `cache_root/` (`index.sqlite` + `bulk.sqlite`); `user.sqlite` under `data_root` (default `$XDG_STATE_HOME/thumtoo`). No legacy `Database` / `BlobStore`. |

| Path | Behaviour |
|------|-----------|
| Size probe | Store blob/media/region only |
| Tiles encode | Store `put_tile` |
| Soft PreferCache | TileSynth / session pixels when tiles cover |
| Soft EnsurePixels | Session encode + TileSynth; **no** durable soft/`levels` writes |
| Tags | Store `blob_tag` for pure `sha256:` / `blob:sha256:` |
| Directory | `Client::refresh_directory_snapshot` / list → Store |
| Archive TOC | Store `container_member` |

On-disk migrate (once, at open): classic dual-path trees move under
`cache_root/legacy/`; redesign files under `store/` move up to the cache root.
Covered by `test_store_root`. Nested `store/` layout and `THUMTOO_STORE_ROOT=0`
are gone.

**biltoo:** pass `$XDG_DATA_HOME/thumtoo` as `data_root` (tip biltoo-1002+).

---

## 2. Host tile-native checklist

1. **Filmstrip / SoftOnly** — Prefer TileSynth from
   `get_pixels(..., allow_tile_synth=true)` / `get_pixels_from_tiles`.
   Do not require a soft `levels` row for “have pixels”.
2. **PreferCache / Overview** — Treat TileSynth as a valid PreferCache delivery.
   Host climb SM must not treat “no soft level row” as permanent miss when
   tiles cover.
3. **Full / crop** — Full EnsurePixels replies via session encode and/or
   TileSynth when the pyramid covers the want.
4. **Gallery** — Overview edge may be TileSynth; escalate to Full as today.
5. **Verify** with a fresh Store cache (no legacy): filmstrip and PreferCache
   still paint within budget.

Normative climb ownership stays in biltoo
[THUMTOO_HOST_CONTRACT.md](../../biltoo/docs/THUMTOO_HOST_CONTRACT.md) (external
tree); do not invent a second PreferCache retry loop.

---

## 3. Library facts (post ≥262)

1. **Client is Store-only** — no legacy ladder classes (≥272). `THUMTOO_STORE_ONLY` is ignored.
2. **Dual-write is gone** — probe/tiles write only to Store.
3. **`has_legacy()` and `Client::db()` are removed** (≥268 / ≥274).
4. Durable tiles go through Store (`put_tile` / tile list APIs).
5. Public APIs are Store-backed: `prepare_paths`, archive TOC, purge, tags, etc.
6. Migration: **no** ladder→tile conversion (PLAN non-goal); cold rebuild tiles.

EnsurePixels does not write durable soft or full_native **levels**. Session
encode + TileSynth supply the reply. Older caches may still contain soft
ladder rows under `legacy/`; new work does not add them.

---

## 4. Environment variables

| Variable | Effect |
|----------|--------|
| **`THUMTOO_STORE_ROOT`** | **Ignored.** Store always at `$cache/`; dual-path migrate still parks classic trees under `$cache/legacy/`. |
| **`THUMTOO_STORE_ONLY`** | **Ignored** (≥262). Client is always Store-only. |
| **`THUMTOO_SOFT_LEVELS`** / **`THUMTOO_TILES_ONLY`** | **Ignored** for Client writes (≥265). Durable soft/`levels` path removed with dual-path handlers. Tiles + session encode remain. |

Pair with biltoo ≥1007 (`scheduleSoftPixels`) for PreferCache when tiles exist.

---

## 5. Status

| Item | State |
|------|--------|
| Library dual-path | **Removed** from Client (≥262) |
| biltoo `data_root` | **Done** (biltoo-1004; XDG data root) |
| biltoo tile-native PreferCache / filmstrip | **OK** (≥1007 `scheduleSoftPixels`; docs biltoo-1012; needs thumtoo ≥280) |
| Tiles-first (no durable soft levels) | **Always** (Client; ≥265 dead env removed) |
| Top-level Store layout | **Always** (nested `store/` opt-out removed; migrate ≥238; `test_store_root`) |
| Full-from-tiles (EnsurePixels Full) | **On** when tile pyramid covers want (≥243) |
| Store-only (Client) | **Always** (≥262) |
| Database / BlobStore sources | **Deleted** (≥272) |
| `has_legacy` / `Client::db` | **Removed** (≥274) |

Legacy `Database` / `BlobStore` classes were **deleted** (≥272). Client never
opened them after ≥262; tools are Store-only. On-disk `legacy/` trees may still
be relocated by layout migrate but are otherwise ignored.

Store maintenance:

```bash
thumtoo-status --cache "$XDG_CACHE_HOME/thumtoo"
thumtoo-gc --cache "$XDG_CACHE_HOME/thumtoo" --store-summary
thumtoo-gc --cache "$XDG_CACHE_HOME/thumtoo" --uri 'file:///path/to/image.jpg'
# optional: --dry-run
```

Schema-4 ladder flags (`--soft-levels`, `--dead-paths`, `--min-scale`) were
removed. Store GC supports `--uri`, `--path`, `--uri-prefix`, `--orphans`, and
`--store-summary` (see tip ≥275).
