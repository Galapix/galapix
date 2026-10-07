// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/store.hpp"
#include "thumtoo/constants.hpp"

#include "sqlite3.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

static int g_failed = 0;

static void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << '\n';
    ++g_failed;
  }
}

int main() {
  const fs::path root =
      fs::temp_directory_path() / "thumtoo-test-store-XXXXXX";
  // mkdtemp needs a mutable buffer ending in XXXXXX
  std::string tmpl = root.string();
  if (tmpl.size() < 6) {
    std::cerr << "temp path too short\n";
    return 1;
  }
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  if (!mkdtemp(buf.data())) {
    std::perror("mkdtemp");
    return 1;
  }
  const fs::path dir(buf.data());

  try {
    {
      auto store = thumtoo::Store::open(dir);
      expect(store.index_schema_version() == thumtoo::kStoreIndexSchemaVersion,
             "schema version 100");
      expect(store.meta_get(thumtoo::kSchemaMetaVersionKey).has_value(),
             "meta version present");

      const auto blob_id =
          store.insert_blob(12345, thumtoo::BlobStatus::Ok);
      expect(blob_id >= 1, "blob id assigned");
      auto blob = store.find_blob(blob_id);
      expect(blob.has_value(), "find blob");
      expect(blob && blob->size && *blob->size == 12345, "blob size");

      // 32-byte fake SHA-256
      std::vector<std::uint8_t> digest(32);
      for (int i = 0; i < 32; ++i) digest[static_cast<std::size_t>(i)] =
          static_cast<std::uint8_t>(i + 1);
      store.put_hash(blob_id, thumtoo::HashAlgoId::Sha256, digest);

      auto found = store.find_blob_by_hash(thumtoo::HashAlgoId::Sha256, digest);
      expect(found && *found == blob_id, "find by hash");

      auto ref = store.blob_ref_sha256(blob_id);
      expect(ref.has_value(), "blob ref");
      expect(ref && ref->starts_with("blob:sha256:"), "blob ref prefix");
      expect(ref && ref->size() == std::string("blob:sha256:").size() + 64,
             "blob ref length");

      auto parsed = thumtoo::Store::parse_sha256_digest(*ref);
      expect(parsed && *parsed == digest, "parse blob ref");

      const std::string uri = "file:///tmp/example.jpg";
      const auto loc_id = store.upsert_locator(uri, blob_id, 12345, 99,
                                              std::string("/tmp/example.jpg"),
                                              std::nullopt);
      expect(loc_id >= 1, "locator id");
      auto loc = store.find_locator(uri);
      expect(loc && loc->blob_id && *loc->blob_id == blob_id, "locator blob");
      expect(loc && loc->outer_path && *loc->outer_path == "/tmp/example.jpg",
             "locator outer_path");
      auto by_outer = store.list_locators_by_outer_path_prefix("/tmp/", 10);
      expect(by_outer.size() >= 1, "list by outer_path prefix");
      expect(store.count_blobs() == 1, "one blob");
      expect(store.count_locators() == 1, "one locator");

      auto locs = store.list_locators_for_blob(blob_id);
      expect(locs.size() == 1 && locs[0].uri == uri, "list locators");

      // Phase B: image media + full region + tile
      const auto media_id = store.ensure_image_media(blob_id, 800, 600);
      expect(media_id >= 1, "media id");
      expect(store.ensure_image_media(blob_id, 800, 600) == media_id,
             "ensure image idempotent");
      auto media = store.find_media(media_id);
      expect(media && media->kind == thumtoo::MediaKind::Image, "media kind");
      expect(media && media->width && *media->width == 800, "media width");
      auto full = store.find_full_region(media_id);
      expect(full.has_value(), "full region");
      expect(full && full->kind == thumtoo::RegionKind::Full, "region full");
      expect(store.count_regions() == 1, "one region");

      thumtoo::Store::TileRow tile;
      tile.media_id = media_id;
      tile.region_id = full->id;
      tile.scale = 0;
      tile.x = 0;
      tile.y = 0;
      tile.width = 256;
      tile.height = 256;
      tile.codec_id = thumtoo::CodecId::Jpeg;
      tile.quality = 85;
      const std::vector<std::uint8_t> payload = {0xff, 0xd8, 0x00, 0x01, 0x02};
      store.put_tile(tile, payload);
      expect(store.has_tile(media_id, full->id, 0, 0, 0), "has tile");
      auto meta = store.find_tile_meta(media_id, full->id, 0, 0, 0);
      expect(meta && meta->width == 256, "tile meta width");
      auto data = store.get_tile_data(media_id, full->id, 0, 0, 0);
      expect(data && *data == payload, "tile payload");
      expect(store.count_tiles() == 1, "one tile");

      auto tile_rows = store.list_tiles_for_region(media_id, full->id);
      expect(tile_rows.size() == 1 && tile_rows[0].scale == 0, "list tiles");
      auto scales = store.list_tile_scales(media_id, full->id);
      expect(scales.size() == 1 && scales[0] == 0, "tile scales");

      // Document page region (no tiles required)
      const auto doc_blob = store.insert_blob(999, thumtoo::BlobStatus::Ok);
      const auto doc_media = store.insert_media(
          doc_blob, thumtoo::MediaKind::Document, {}, {},
          thumtoo::MediaStatus::Ready);
      store.set_media_page_count(doc_media, 10);
      const auto page_r =
          store.ensure_region(doc_media, thumtoo::RegionKind::Page, "3", 3);
      auto page = store.find_region(page_r);
      expect(page && page->key == "3", "page region key");

      // Phase C: container members (TOC without hash) + document helpers
      const auto zip_blob = store.insert_blob(50, thumtoo::BlobStatus::Ok);
      store.upsert_container_member(zip_blob, "a.jpg", false, 1000, {});
      store.upsert_container_member(zip_blob, "subdir/", true, {}, {});
      expect(store.count_container_members(zip_blob) == 2, "two members");
      auto mem = store.find_container_member(zip_blob, "a.jpg");
      expect(mem && !mem->blob_id, "member unhashed");
      const auto member_blob =
          store.insert_blob(1000, thumtoo::BlobStatus::Ok);
      store.set_container_member_blob(zip_blob, "a.jpg", member_blob);
      mem = store.find_container_member(zip_blob, "a.jpg");
      expect(mem && mem->blob_id && *mem->blob_id == member_blob,
             "member hashed");

      // TOC refresh preserves hash via upsert coalesce when blob_id omitted
      store.upsert_container_member(zip_blob, "a.jpg", false, 1000, {});
      mem = store.find_container_member(zip_blob, "a.jpg");
      expect(mem && mem->blob_id && *mem->blob_id == member_blob,
             "hash preserved on toc upsert");

      const auto dmedia = store.ensure_document_media(doc_blob, 5);
      expect(dmedia == doc_media, "ensure document idempotent");
      const auto p1 = store.ensure_page_region(dmedia, 1);
      expect(store.find_region(p1)->key == "1", "page 1 key");

      // Phase D: directory snapshot (cache-first list)
      thumtoo::Store::DirectorySnapshotRow snap;
      snap.dir_uri = "file:///tmp/photos";
      snap.size = 2;
      snap.mtime_ns = 1000;
      snap.incomplete = false;
      std::vector<thumtoo::Store::DirectoryEntryRow> entries;
      {
        thumtoo::Store::DirectoryEntryRow e;
        e.name = "a.jpg";
        e.child_uri = "file:///tmp/photos/a.jpg";
        e.is_dir = false;
        e.size = 12345;
        entries.push_back(std::move(e));
      }
      {
        thumtoo::Store::DirectoryEntryRow e;
        e.name = "subdir";
        e.child_uri = "file:///tmp/photos/subdir";
        e.is_dir = true;
        entries.push_back(std::move(e));
      }
      store.replace_directory_snapshot(snap, entries);
      expect(store.count_directory_snapshots() == 1, "one dir snapshot");
      auto got_snap = store.find_directory_snapshot(snap.dir_uri);
      expect(got_snap && got_snap->size && *got_snap->size == 2, "snap size");
      auto listed = store.list_directory_entries(snap.dir_uri);
      expect(listed.size() == 2, "two dir entries");
      expect(listed[0].name == "a.jpg" && !listed[0].is_dir, "entry file");
      expect(listed[1].name == "subdir" && listed[1].is_dir, "entry dir");
      // Replace shrinks entries
      store.replace_directory_snapshot(snap, {entries[0]});
      expect(store.list_directory_entries(snap.dir_uri).size() == 1,
             "replace shrinks");

      // Phase D: user tags survive by blob_ref
      const std::string bref = *ref;
      store.add_blob_tag(bref, "favorite", "user");
      store.add_blob_tag(bref, "vacation", "user");
      auto tags = store.tags_for_blob_ref(bref);
      expect(tags.size() == 2, "two tags");
      expect(tags[0] == "favorite" && tags[1] == "vacation", "tag order");
      auto defs = store.find_tag_def_by_name("favorite");
      expect(defs.has_value(), "tag_def exists");
      expect(store.ensure_tag_def("favorite") == defs->id, "ensure tag idempotent");
      auto refs = store.blob_refs_for_tag("favorite");
      expect(refs.size() == 1 && refs[0] == bref, "blob_refs_for_tag");
      expect(store.remove_blob_tag(bref, "vacation"), "remove tag");
      expect(store.tags_for_blob_ref(bref).size() == 1, "one tag left");
      expect(store.tags_for_blob_ref(bref)[0] == "favorite", "favorite remains");

      // Phase D: collections, bookmarks, link edges
      const auto coll =
          store.create_collection(std::string_view{"Album"}, std::string_view{"#f00"});
      expect(coll >= 1, "collection id");
      expect(store.count_collections() == 1, "one collection");
      store.upsert_collection_member(coll, bref, 0, std::string_view{"/tmp/a.jpg"});
      store.upsert_collection_member(coll, bref, 1);  // update ordinal
      auto members = store.list_collection_members(coll);
      expect(members.size() == 1 && members[0].ordinal && *members[0].ordinal == 1,
             "collection member upsert");
      expect(store.remove_collection_member(coll, bref), "remove member");
      expect(store.list_collection_members(coll).empty(), "no members");
      store.upsert_collection_member(coll, bref, 0);

      const auto bm =
          store.create_bookmark(bref, std::string_view{"Chapter 1"});
      expect(store.count_bookmarks() == 1, "one bookmark");
      auto bmrow = store.find_bookmark(bm);
      expect(bmrow && bmrow->title && *bmrow->title == "Chapter 1", "bm title");
      store.set_bookmark_title(bm, std::string_view{"Ch. 1"});
      expect(store.list_bookmarks_for_target(bref).size() == 1, "list bm");

      const std::string other = "blob:sha256:" + std::string(64, 'a');
      const auto edge =
          store.add_link_edge(bref, other, std::string_view{"see-also"});
      expect(store.add_link_edge(bref, other, std::string_view{"see-also"}) ==
                 edge,
             "link idempotent");
      expect(store.list_links_from(bref).size() == 1, "links from");
      expect(store.list_links_to(other).size() == 1, "links to");
      expect(store.remove_link_edge(edge), "remove link");
      expect(store.list_links_from(bref).empty(), "no links");
      // re-add for wipe survival
      (void)store.add_link_edge(bref, other, std::string_view{"see-also"});

      // Annotation + http_body
      const std::vector<std::uint8_t> geom = {1, 2, 3};
      const auto ann = store.create_annotation(bref, 1, std::string_view{"note"},
                                               geom);
      auto arow = store.find_annotation(ann);
      expect(arow && arow->body && *arow->body == "note", "annotation body");
      expect(arow && arow->geom == geom, "annotation geom");
      expect(store.list_annotations_for_target(bref).size() == 1, "list ann");
      store.set_annotation_body(ann, std::string_view{"edited"});
      expect(store.find_annotation(ann)->body &&
                 *store.find_annotation(ann)->body == "edited",
             "ann body update");

      const std::vector<std::uint8_t> http = {'h', 'i'};
      store.put_http_body("https://example.com/x", http, blob_id);
      auto hb = store.get_http_body("https://example.com/x");
      expect(hb && hb->data == http && hb->blob_id && *hb->blob_id == blob_id,
             "http body");
      expect(store.delete_http_body("https://example.com/x"), "delete http");
      expect(!store.get_http_body("https://example.com/x"), "http gone");
      // re-put for wipe: bulk is wiped with index, so no survival expected
    }

    // Re-open preserves rows
    {
      auto store = thumtoo::Store::open(dir);
      expect(store.count_blobs() == 4, "reopen blobs");
      expect(store.count_locators() == 1, "reopen locators");
      expect(store.count_directory_snapshots() == 1, "reopen dir snap");
      std::vector<std::uint8_t> digest(32);
      for (int i = 0; i < 32; ++i)
        digest[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i + 1);
      auto bref = thumtoo::Store::format_blob_ref_sha256(digest);
      expect(store.tags_for_blob_ref(bref).size() == 1, "reopen tags");
      expect(store.count_collections() == 1, "reopen collections");
      expect(store.count_bookmarks() == 1, "reopen bookmarks");
      expect(store.list_links_from(bref).size() == 1, "reopen links");
    }

    // forget_uri: dedicated locator so other fixtures stay intact
    {
      auto store = thumtoo::Store::open(dir);
      const std::string forget_uri = "file:///tmp/forget-me.jpg";
      std::vector<std::uint8_t> dig(32, 0xab);
      const auto bid = store.insert_blob(10, thumtoo::BlobStatus::Ok);
      store.put_hash(bid, thumtoo::HashAlgoId::Sha256, dig);
      (void)store.upsert_locator(forget_uri, bid, 10, 1);
      const auto mid = store.ensure_image_media(bid, 16, 16);
      auto full = store.find_full_region(mid);
      expect(full.has_value(), "forget fixture region");
      thumtoo::Store::TileRow tr;
      tr.media_id = mid;
      tr.region_id = full->id;
      tr.scale = 0;
      tr.x = 0;
      tr.y = 0;
      tr.width = 16;
      tr.height = 16;
      tr.codec_id = thumtoo::CodecId::Jpeg;
      const std::vector<std::uint8_t> pay = {1, 2, 3};
      store.put_tile(tr, pay);
      auto dry = store.forget_uri(forget_uri, true);
      expect(dry.locator_removed && dry.blob_purged, "dry forget");
      auto st = store.forget_uri(forget_uri, false);
      expect(st.locator_removed && st.blob_purged, "forget purged");
      expect(st.tiles_deleted >= 1, "forget tiles");
      expect(!store.find_locator(forget_uri), "forget locator gone");
      expect(!store.find_blob(bid), "forget blob gone");

      // Orphan blob (no locator): purge_orphan_blobs
      const auto orphan = store.insert_blob(7, thumtoo::BlobStatus::Ok);
      expect(store.list_orphan_blob_ids().size() >= 1, "list orphans");
      auto ost = store.purge_orphan_blobs(/*dry_run=*/false);
      expect(ost.blobs_purged >= 1, "purge orphan count");
      expect(!store.find_blob(orphan), "orphan blob gone");

      // forget_uri_prefix
      {
        const auto b1 = store.insert_blob(1, thumtoo::BlobStatus::Ok);
        const auto b2 = store.insert_blob(2, thumtoo::BlobStatus::Ok);
        (void)store.upsert_locator("file:///tmp/dir/a.jpg", b1, 1, 1);
        (void)store.upsert_locator("file:///tmp/dir/b.jpg", b2, 2, 2);
        (void)store.upsert_locator("file:///tmp/other.jpg", b2, 2, 2);
        auto pst = store.forget_uri_prefix("file:///tmp/dir/", false);
        expect(pst.locators_removed == 2, "prefix removes two");
        expect(!store.find_locator("file:///tmp/dir/a.jpg"), "a gone");
        expect(store.find_locator("file:///tmp/other.jpg").has_value(),
               "other kept");
      }

      // blob_lqip durable placeholder (page-aware)
      {
        const auto bid = store.insert_blob(3, thumtoo::BlobStatus::Ok);
        const std::vector<std::uint8_t> hash = {1, 2, 3, 4, 5};
        store.put_blob_lqip(bid, 1, hash, 0);
        auto got = store.get_blob_lqip(bid, 0);
        expect(got && *got == hash, "lqip roundtrip");
        expect(store.get_blob_lqip_kind(bid, 0) && *store.get_blob_lqip_kind(bid, 0) == 1,
               "lqip kind");
        const std::vector<std::uint8_t> page = {9, 9, 9};
        store.put_blob_lqip(bid, 1, page, 3);
        auto gp = store.get_blob_lqip(bid, 3);
        expect(gp && *gp == page, "lqip page 3");
        expect(store.get_blob_lqip(bid, 0) && *store.get_blob_lqip(bid, 0) == hash,
               "page 0 independent");
      }

      // page_text_layer + document_outline
      {
        const auto bid = store.insert_blob(9, thumtoo::BlobStatus::Ok);
        const std::vector<std::uint8_t> payload = {9, 8, 7};
        store.put_page_text_layer(bid, 2, "k", payload);
        auto got = store.get_page_text_layer(bid, 2, "k");
        expect(got && *got == payload, "text layer roundtrip");
        store.put_document_outline(bid, "", payload);
        auto og = store.get_document_outline(bid, "");
        expect(og && *og == payload, "outline roundtrip");
      }

      // list_locators_like
      {
        auto rows = store.list_locators_like("file:///tmp/%", 50);
        expect(rows.size() >= 1, "like list finds file uris");
      }
    }

    // Pre-outer_path schema 100 index: locator without outer_path/member_path.
    // Open must ALTER-add columns so upsert/find do not throw "no such column".
    {
      const fs::path old_dir =
          fs::temp_directory_path() / "thumtoo-test-store-oldloc-XXXXXX";
      std::string otmpl = old_dir.string();
      std::vector<char> obuf(otmpl.begin(), otmpl.end());
      obuf.push_back('\0');
      if (!mkdtemp(obuf.data())) {
        std::perror("mkdtemp oldloc");
        ++g_failed;
      } else {
        const fs::path odir(obuf.data());
        const fs::path index_path = odir / "index.sqlite";
        sqlite3* db = nullptr;
        if (sqlite3_open(index_path.string().c_str(), &db) != SQLITE_OK) {
          expect(false, "seed old locator open");
        } else {
          char* err = nullptr;
          const char* sql =
              "CREATE TABLE schema_meta(key TEXT PRIMARY KEY, value TEXT NOT NULL);"
              "INSERT INTO schema_meta(key, value) VALUES('schema_version', '100');"
              "CREATE TABLE blob("
              "  id INTEGER PRIMARY KEY, size INTEGER, status INTEGER NOT NULL DEFAULT 0,"
              "  created_at INTEGER NOT NULL, updated_at INTEGER NOT NULL);"
              "CREATE TABLE locator("
              "  id INTEGER PRIMARY KEY, uri TEXT NOT NULL UNIQUE,"
              "  blob_id INTEGER, size INTEGER, mtime_ns INTEGER,"
              "  updated_at INTEGER NOT NULL);";
          if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            std::cerr << "seed old locator: " << (err ? err : "?") << "\n";
            sqlite3_free(err);
            expect(false, "seed old locator sql");
          }
          sqlite3_close(db);
          try {
            auto store = thumtoo::Store::open(odir);
            const auto blob_id =
                store.insert_blob(7, thumtoo::BlobStatus::Ok);
            const auto loc_id = store.upsert_locator(
                "file:///tmp/pre-outer.jpg", blob_id, 7, 1,
                std::string("/tmp/pre-outer.jpg"), std::nullopt);
            expect(loc_id >= 1, "old-schema upsert_locator after migrate");
            auto loc = store.find_locator("file:///tmp/pre-outer.jpg");
            expect(loc.has_value(), "old-schema find_locator");
            expect(loc && loc->outer_path &&
                       *loc->outer_path == "/tmp/pre-outer.jpg",
                   "old-schema outer_path populated");
            auto by_prefix = store.list_locators_by_outer_path_prefix("/tmp/", 10);
            expect(by_prefix.size() >= 1, "old-schema list by outer_path");
          } catch (const std::exception& ex) {
            std::cerr << "old-schema migrate exception: " << ex.what() << "\n";
            ++g_failed;
          }
        }
        std::error_code oec;
        fs::remove_all(odir, oec);
      }
    }

    // Re-create after deleting index+bulk (simulates operator cache wipe).
    // User DB must keep tags / collections / bookmarks / links.
    {
      fs::remove(dir / "index.sqlite");
      fs::remove(dir / "bulk.sqlite");
      auto store = thumtoo::Store::open(dir);
      expect(store.index_schema_version() == thumtoo::kStoreIndexSchemaVersion,
             "fresh after remove");
      expect(store.count_blobs() == 0, "empty after wipe");
      expect(store.count_directory_snapshots() == 0, "dir snap wiped with index");
      std::vector<std::uint8_t> digest(32);
      for (int i = 0; i < 32; ++i)
        digest[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(i + 1);
      auto bref = thumtoo::Store::format_blob_ref_sha256(digest);
      expect(store.tags_for_blob_ref(bref).size() == 1, "tags survive wipe");
      expect(store.tags_for_blob_ref(bref)[0] == "favorite", "favorite survives");
      expect(store.count_collections() == 1, "collections survive wipe");
      expect(store.count_bookmarks() == 1, "bookmarks survive wipe");
      expect(store.list_links_from(bref).size() == 1, "links survive wipe");
    }
  } catch (const std::exception& ex) {
    std::cerr << "exception: " << ex.what() << '\n';
    ++g_failed;
  }

  std::error_code ec;
  fs::remove_all(dir, ec);

  if (g_failed) {
    std::cerr << g_failed << " failure(s)\n";
    return 1;
  }
  std::cout << "ok\n";
  return 0;
}
