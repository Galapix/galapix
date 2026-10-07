-- SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
-- SPDX-License-Identifier: GPL-3.0-or-later
-- thumtoo schema_version 4

CREATE TABLE IF NOT EXISTS schema_meta (
  key TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS content (
  content_id TEXT PRIMARY KEY,
  width INTEGER,
  height INTEGER,
  format TEXT,
  duration_ms INTEGER,
  still_count INTEGER,
  status INTEGER NOT NULL DEFAULT 0,
  error_code TEXT,
  updated_at INTEGER,
  lqip BLOB,
  lqip_kind INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS locators (
  uri TEXT PRIMARY KEY,
  content_id TEXT,
  outer_path TEXT,
  member_path TEXT,
  size INTEGER,
  mtime_ns INTEGER,
  updated_at INTEGER
);

CREATE TABLE IF NOT EXISTS archive_entries (
  archive_uri TEXT NOT NULL,
  member_path TEXT NOT NULL,
  uncompressed_size INTEGER,
  PRIMARY KEY (archive_uri, member_path)
);

CREATE TABLE IF NOT EXISTS directory_snapshots (
  dir_uri TEXT PRIMARY KEY,
  size INTEGER,
  mtime_ns INTEGER,
  listed_at INTEGER,
  incomplete INTEGER
);

CREATE TABLE IF NOT EXISTS directory_entries (
  dir_uri TEXT NOT NULL,
  name TEXT NOT NULL,
  child_uri TEXT,
  is_dir INTEGER,
  size INTEGER,
  mtime_ns INTEGER,
  PRIMARY KEY (dir_uri, name)
);

CREATE TABLE IF NOT EXISTS levels (
  content_id TEXT NOT NULL,
  max_edge INTEGER NOT NULL,
  frame_idx INTEGER NOT NULL DEFAULT 0,
  pts_ms INTEGER,
  width INTEGER,
  height INTEGER,
  codec TEXT,
  quality INTEGER,
  path TEXT,
  PRIMARY KEY (content_id, max_edge, frame_idx)
);

CREATE TABLE IF NOT EXISTS tags (
  content_id TEXT NOT NULL,
  tag TEXT NOT NULL,
  source TEXT,
  created_at INTEGER,
  PRIMARY KEY (content_id, tag)
);

CREATE TABLE IF NOT EXISTS tiles (
  content_id TEXT NOT NULL,
  scale INTEGER NOT NULL,
  x INTEGER NOT NULL,
  y INTEGER NOT NULL,
  width INTEGER,
  height INTEGER,
  codec TEXT,
  quality INTEGER,
  source INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (content_id, scale, x, y)
);

CREATE INDEX IF NOT EXISTS idx_locators_content_id ON locators(content_id);
CREATE INDEX IF NOT EXISTS idx_locators_outer_path ON locators(outer_path);
CREATE INDEX IF NOT EXISTS idx_levels_content_id ON levels(content_id);
CREATE INDEX IF NOT EXISTS idx_tiles_content_id ON tiles(content_id);

CREATE TABLE IF NOT EXISTS text_layers (
  content_id TEXT NOT NULL,
  page_1based INTEGER NOT NULL,
  layout_key TEXT NOT NULL DEFAULT '',
  page_x0 REAL,
  page_y0 REAL,
  page_x1 REAL,
  page_y1 REAL,
  payload BLOB NOT NULL,
  updated_at INTEGER,
  PRIMARY KEY (content_id, page_1based, layout_key)
);

CREATE TABLE IF NOT EXISTS document_outlines (
  content_id TEXT NOT NULL,
  layout_key TEXT NOT NULL DEFAULT '',
  payload BLOB NOT NULL,
  updated_at INTEGER,
  PRIMARY KEY (content_id, layout_key)
);

CREATE INDEX IF NOT EXISTS idx_text_layers_content_id ON text_layers(content_id);

CREATE TABLE IF NOT EXISTS document_index (
  document_uri TEXT NOT NULL,
  layout_key TEXT NOT NULL DEFAULT '',
  page_count INTEGER NOT NULL,
  size INTEGER,
  mtime_ns INTEGER,
  indexed_at INTEGER,
  PRIMARY KEY (document_uri, layout_key)
);

