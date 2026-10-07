<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Architecture

[DESIGN.md](DESIGN.md) is normative for product rules (ladder schema sections are
**historical** — live path is redesign Store ≥100). This file tracks the tree.

## Tree

```text
thumtoo/
  README.md DESIGN.md ARCHITECTURE.md TODO.md AGENTS.md
  REUSE.toml LICENSES/
  flake.nix CMakeLists.txt
  include/thumtoo/
    client.hpp store.hpp constants.hpp types.hpp uri.hpp …
  src/
    client.cpp store.cpp image.cpp layout.cpp uri.cpp …
  tools/
    thumtoo_status.cpp thumtoo_prepare.cpp thumtoo_gc.cpp
  tests/
    test_store.cpp test_client.cpp test_store_root.cpp …
  external/   # optional vendored sources (not system libs)
```

## Build

```bash
nix develop          # optional
cmake -B build -GNinja
cmake --build build
ctest --test-dir build
./build/thumtoo-status --cache /path/to/cache
./build/thumtoo-gc --cache /path/to/cache --store-summary
```

## Cache layout (default `THUMTOO_STORE_ROOT=on`)

```text
$XDG_CACHE_HOME/thumtoo/
  index.sqlite   # Store meta (schema ≥ 100)
  bulk.sqlite    # tile / payload bulk
  legacy/        # optional relocated pre-cutover files (ignored by Client)
```

User data (tags, etc.): `$XDG_DATA_HOME/thumtoo/user.sqlite` when `data_root` is set.

## Host contract

See [docs/HOST_CUTOVER.md](docs/HOST_CUTOVER.md) and
[docs/API_MIGRATION.md](docs/API_MIGRATION.md). Client is Store-only; schema-4
`Database` / `BlobStore` were removed.
