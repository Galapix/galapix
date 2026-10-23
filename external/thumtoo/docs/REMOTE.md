<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Remote cache: thumtoo-serve and Client::open_remote

Browse a thumtoo cache on another machine. `thumtoo-serve` exposes a cache
read-only over HTTP; `Client::open_remote(url)` is a `thumtoo::Client` whose
reads go to that server, so hosts (galapix) keep their Client code.

Scope: locations, their metadata (size, format, tile coverage, LQIP) and
**stored** tiles. Nothing is generated remotely and nothing under
`$XDG_STATE_HOME/thumtoo` (tags, collections) is served.

## Server

```bash
thumtoo-serve                                   # 127.0.0.1:8080, no token
thumtoo-serve --listen 0.0.0.0:8080             # prints http://HOST:8080/?token=…
thumtoo-serve --listen 0.0.0.0:8080 --token-file ~/.config/thumtoo/serve-token \
              --allow-prefix file:///home/ingo/Pictures/
```

- Opens the cache with `Store::open_readonly` (one connection per request
  thread). Safe next to a live writer; refuses a cache of another
  schema_version.
- **Auth:** loopback binds need no token. Any other bind gets a random
  128-bit token per start (or `--token-file`, created if missing); `--no-auth`
  turns it off. Accepted as `?token=` (sets an HttpOnly cookie),
  `Authorization: Bearer …`, or the `thumtoo_token` cookie.
- **No TLS.** Plain HTTP; encrypt with `ssh -L 8080:localhost:8080 host`,
  Tailscale/WireGuard or a reverse proxy.
- `--allow-prefix` (repeatable) limits listings, `//info`, `//tiles` and
  location tile routes to URIs below the prefixes. `/blob/<content_id>` tile
  routes are not filtered (ids are content hashes taken from allowed `//info`).

## Routes (GET, API 1)

Locations are thumtoo location URIs written into the path as-is, with bytes
outside unreserved and `/:,;=@!$&'()*+` percent-escaped (`format_remote_route`).

| Route | Response |
|-------|----------|
| `/` | `{"service":"thumtoo-serve","api":1,"version":…,"tile_size":256}` |
| `/locations?prefix=&like=&after=&limit=` | `{"locations":[info…],"next":uri\|null}`; `like` is SQL LIKE; page with `after=next`; `limit` ≤ 5000 |
| `/<location>//info` | info object (below); 404 if unknown |
| `/<location>//tiles` | `{"tiles":[{scale,x,y,width,height,codec}…]}` |
| `/<location>//tile:S,X,Y` | tile bytes (`image/jpeg`, `image/png`, `image/jxl`), `X-Tile-Width/Height`, `ETag`, `Cache-Control: no-cache`; 404 if not stored |
| `/blob/<content_id>//tile:S,X,Y` | same, `Cache-Control: immutable` |

Info object: `uri`, `outer_path`, `member_path`, `file_size`, `mtime_ns`,
`content_id`, `status`, `format`, `width`, `height`, `min_scale`,
`max_scale`, `lqip` (base64). Fields are omitted when unknown.

Example: `/file:///home/ingo/a.pdf//page:3//tile:0,1,2`.

## Client::open_remote

```cpp
auto client = thumtoo::Client::open_remote("http://host:8080/?token=…", executor);
```

Throws `std::runtime_error` when the server is unreachable, rejects the token
or speaks another API version. `thumtoo::is_remote_cache_url(s)` tells a URL
from a cache directory so hosts can accept either in one option.

Same threading contract as a local Client:

| Call | Remote behaviour |
|------|------------------|
| `get_size` / `get_meta` / `get_tile_coverage` / `get_lqip` | In-RAM memo only (never network). Filled by `list_locators*`, `find_locator` and `request_size`. |
| `list_locators*`, `find_locator` | Blocking HTTP (`/locations` paged by 2000, `//info`); call off the frame loop |
| `request_size` | Worker fetches `//info`; empty `SizeReply` on 404 |
| `request_tile_cells` | Worker fetches each cell; decoded to rgb888 like local cells. 404 → `Unavailable` ("not in the remote cache"), transport/HTTP errors → `Failed` |
| `request_tile` | Same fetch, legacy `nullopt` on miss; pyramids answer the miss |
| `get_tile` / `has_tile` | Miss (no tile memo; use `request_tile_cells`) |
| `request_pixels` & co. | Miss (whole-image rasters are not served) |
| `store()`, tags, prepare, purge | Local only; do not call on a remote Client |

`list_locators_by_outer_path_prefix(path)` maps to the URI prefix
`file_uri_from_path(path)` on the server's paths (no local stat).

Build: remote support needs libcurl + nlohmann_json (`THUMTOO_HAVE_REMOTE`);
`thumtoo-serve` additionally cpp-httplib (`THUMTOO_BUILD_SERVE`, on when
found). Without them `open_remote` throws and `remote_available()` is false.

Tests: `tests/test_remote.cpp` (spawns thumtoo-serve on a temp cache),
`tests/test_cache_reader.cpp`.
