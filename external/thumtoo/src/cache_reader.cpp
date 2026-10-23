// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/cache_reader.hpp"

#include "thumtoo/constants.hpp"
#include "thumtoo/djvu.hpp"
#include "thumtoo/epub.hpp"
#include "thumtoo/lqip.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/uri.hpp"

#include <string>

namespace thumtoo {
namespace {

[[nodiscard]] bool parse_content_id_suffix(std::string_view rest_after_hex,
                                           ContentIdParts* out) {
  if (!out) return false;
  *out = ContentIdParts{};
  if (rest_after_hex.empty()) return true;
  constexpr std::string_view kPage = ":page:";
  constexpr std::string_view kPdfImage = ":pdfimage:";
  constexpr std::string_view kEpub = ":epub:";
  if (rest_after_hex.size() > kPage.size() &&
      rest_after_hex.substr(0, kPage.size()) == kPage) {
    const std::string_view after = rest_after_hex.substr(kPage.size());
    std::size_t end = 0;
    while (end < after.size() && after[end] >= '0' && after[end] <= '9') ++end;
    if (end == 0) return false;
    try {
      out->page_1based = std::stoi(std::string(after.substr(0, end)));
    } catch (...) {
      return false;
    }
    if (*out->page_1based < 1) return false;
    std::string_view tail = after.substr(end);
    if (tail.size() >= kEpub.size() && tail.substr(0, kEpub.size()) == kEpub) {
      out->epub_layout_key = std::string(tail.substr(kEpub.size()));
      if (out->epub_layout_key->empty()) return false;
      out->page_region_key =
          epub_page_region_key(*out->page_1based, *out->epub_layout_key);
    } else if (!tail.empty()) {
      return false;  // unknown suffix after page number
    } else {
      out->page_region_key = std::to_string(*out->page_1based);
    }
    return true;
  }
  if (rest_after_hex.size() > kPdfImage.size() &&
      rest_after_hex.substr(0, kPdfImage.size()) == kPdfImage) {
    try {
      out->pdfimage_1based =
          std::stoi(std::string(rest_after_hex.substr(kPdfImage.size())));
    } catch (...) {
      return false;
    }
    if (*out->pdfimage_1based < 1) return false;
    return true;
  }
  return false;
}

}  // namespace

std::optional<ContentIdParts> parse_content_id(std::string_view content_id) {
  constexpr std::string_view kSha = "sha256:";
  if (!content_id.starts_with(kSha) || content_id.size() < kSha.size() + 64) {
    return std::nullopt;
  }
  const std::string_view rest = content_id.substr(kSha.size());
  const std::string_view hex = rest.substr(0, 64);
  for (char c : hex) {
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
          (c >= 'A' && c <= 'F'))) {
      return std::nullopt;
    }
  }
  ContentIdParts parts;
  if (rest.size() > 64 && !parse_content_id_suffix(rest.substr(64), &parts)) {
    return std::nullopt;
  }
  auto digest = Store::parse_sha256_digest(hex);
  if (!digest || digest->size() != 32) return std::nullopt;
  parts.sha256 = std::move(*digest);
  return parts;
}

const char* tile_codec_name(CodecId codec) noexcept {
  switch (codec) {
    case CodecId::Jxl: return "jxl";
    case CodecId::Png: return "png";
    case CodecId::Jpeg: break;
  }
  return kDefaultTileCodec;
}

std::optional<ContentMeta> CacheReader::meta(std::string_view uri) const {
  auto loc = store_->find_locator(uri);
  if (!loc || !loc->blob_id) return std::nullopt;

  auto ref = store_->blob_ref_sha256(*loc->blob_id);
  if (!ref || !ref->starts_with("blob:")) return std::nullopt;
  // "blob:sha256:…" → legacy-style "sha256:…"
  ContentMeta cm;
  cm.content_id = ref->substr(5);

  if (auto pdf = parse_pdf_uri(uri)) {
    auto media =
        store_->find_media_for_blob(*loc->blob_id, MediaKind::Document);
    if (!media) return std::nullopt;
    auto region = store_->find_region_by_key(
        media->id, RegionKind::Page, std::to_string(pdf->page));
    if (!region) return std::nullopt;
    cm.content_id += ":page:" + std::to_string(pdf->page);
    cm.format = "pdf";
    cm.status = ContentStatus::Incomplete;
    if (!store_->list_tile_scales(media->id, region->id).empty()) {
      cm.status = ContentStatus::Ready;
    }
    // One size only: page document layout (region.width/height at probe).
    // Never shared media dims; never invent size from tiles; never open source.
    if (region->width && region->height && *region->width > 0
        && *region->height > 0) {
      cm.size = Size{*region->width, *region->height};
    }
    return cm;
  }

  if (auto pimg = parse_pdf_image_uri(uri)) {
    auto media =
        store_->find_media_for_blob(*loc->blob_id, MediaKind::Document);
    if (!media) return std::nullopt;
    const std::string key = std::to_string(pimg->image);
    auto region =
        store_->find_region_by_key(media->id, RegionKind::Fragment, key);
    if (!region) return std::nullopt;
    cm.content_id += ":pdfimage:" + key;
    cm.format = "pdfimage";
    cm.status = ContentStatus::Incomplete;
    if (!store_->list_tile_scales(media->id, region->id).empty()) {
      cm.status = ContentStatus::Ready;
    }
    // Region size first; media dims only as last Store fallback — no source open.
    if (region->width && region->height && *region->width > 0
        && *region->height > 0) {
      cm.size = Size{*region->width, *region->height};
    } else if (media->width && media->height) {
      cm.size = Size{*media->width, *media->height};
    }
    return cm;
  }

  if (auto dj = parse_djvu_uri(uri)) {
    auto media =
        store_->find_media_for_blob(*loc->blob_id, MediaKind::Document);
    if (!media) return std::nullopt;
    auto region = store_->find_region_by_key(
        media->id, RegionKind::Page, std::to_string(dj->page));
    if (!region) return std::nullopt;
    cm.content_id += ":page:" + std::to_string(dj->page);
    cm.format = "djvu";
    cm.status = ContentStatus::Incomplete;
    if (!store_->list_tile_scales(media->id, region->id).empty()) {
      cm.status = ContentStatus::Ready;
    }
    if (region->width && region->height && *region->width > 0
        && *region->height > 0) {
      cm.size = Size{*region->width, *region->height};
    }
    return cm;
  }

  if (auto ep = parse_epub_uri(uri)) {
    auto media =
        store_->find_media_for_blob(*loc->blob_id, MediaKind::Document);
    if (!media) return std::nullopt;
    const std::string rkey = epub_page_region_key(ep->page, ep->layout);
    auto region = store_->find_region_by_key(
        media->id, RegionKind::Page, rkey);
    if (!region) return std::nullopt;
    // Layout is part of tile identity (docs/EPUB.md): sha256:…:page:N:epub:…
    cm.content_id += epub_content_id_page_suffix(ep->page, ep->layout);
    cm.format = "epub";
    cm.status = ContentStatus::Incomplete;
    if (!store_->list_tile_scales(media->id, region->id).empty()) {
      cm.status = ContentStatus::Ready;
    }
    if (region->width && region->height && *region->width > 0
        && *region->height > 0) {
      cm.size = Size{*region->width, *region->height};
    }
    return cm;
  }

  if (auto media =
          store_->find_media_for_blob(*loc->blob_id, MediaKind::Image)) {
    if (media->width && media->height) {
      cm.size = Size{*media->width, *media->height};
    }
    cm.format = "image";
    cm.status = ContentStatus::Incomplete;
    if (auto full = store_->find_full_region(media->id)) {
      if (!store_->list_tile_scales(media->id, full->id).empty()) {
        cm.status = ContentStatus::Ready;
      }
    }
    return cm;
  }
  return std::nullopt;
}


std::optional<CacheReader::TileTarget> CacheReader::tile_target(
    std::string_view content_id) const {
  auto parsed = parse_content_id(content_id);
  if (!parsed) return std::nullopt;
  const ContentIdParts& parts = *parsed;

  auto blob_id = store_->find_blob_by_hash(HashAlgoId::Sha256, parts.sha256);
  if (!blob_id) return std::nullopt;

  if (parts.page_1based) {
    auto media = store_->find_media_for_blob(*blob_id, MediaKind::Document);
    if (!media) return std::nullopt;
    auto region = store_->find_region_by_key(
        media->id, RegionKind::Page, parts.page_region_key);
    if (!region) return std::nullopt;
    return TileTarget{media->id, region->id};
  }

  if (parts.pdfimage_1based) {
    auto media = store_->find_media_for_blob(*blob_id, MediaKind::Document);
    if (!media) return std::nullopt;
    auto region = store_->find_region_by_key(
        media->id, RegionKind::Fragment, std::to_string(*parts.pdfimage_1based));
    if (!region) return std::nullopt;
    return TileTarget{media->id, region->id};
  }

  auto media = store_->find_media_for_blob(*blob_id, MediaKind::Image);
  if (!media) return std::nullopt;
  auto full = store_->find_full_region(media->id);
  if (!full) return std::nullopt;
  return TileTarget{media->id, full->id};
}

std::optional<TileBlob> CacheReader::tile(const TileTarget& target, int scale,
                                          int x, int y) const {
  auto bytes =
      store_->get_tile_data(target.media_id, target.region_id, scale, x, y);
  if (!bytes) return std::nullopt;
  TileBlob t;
  t.scale = scale;
  t.x = x;
  t.y = y;
  t.codec = kDefaultTileCodec;
  if (auto row = store_->find_tile_meta(target.media_id, target.region_id,
                                        scale, x, y)) {
    t.width = row->width;
    t.height = row->height;
    t.codec = tile_codec_name(row->codec_id);
  }
  // Do not reject on w/h vs expected_tile_cell_wh. JPEG DCT vs floor-half
  // drift is a few pixels; paint stretches the full bitmap into the grid
  // cell. Re-encode-on-mismatch caused miss loops and host Failed cells.
  t.bytes = std::move(*bytes);
  t.source = TileSource::Full;
  return t;
}

std::optional<TileBlob> CacheReader::tile(std::string_view uri, int scale,
                                          int x, int y) const {
  auto m = meta(uri);
  if (!m) return std::nullopt;
  auto tgt = tile_target(m->content_id);
  if (!tgt) return std::nullopt;
  return tile(*tgt, scale, x, y);
}

std::optional<TileCoverage> CacheReader::tile_coverage(
    std::string_view uri) const {
  auto meta = this->meta(uri);
  if (!meta) return std::nullopt;
  TileCoverage cov;
  if (meta->size) cov.size = *meta->size;
  if (auto tgt = tile_target(meta->content_id)) {
    auto scales = store_->list_tile_scales(tgt->media_id, tgt->region_id);
    if (!scales.empty()) {
      cov.min_scale = scales.front();
      cov.max_scale = scales.back();
      return cov;
    }
  }
  // No tiles stored yet: theoretical coverage from native size alone.
  if (!meta->size || meta->size->width <= 0 || meta->size->height <= 0) {
    return std::nullopt;
  }
  cov.min_scale = 0;
  int w = meta->size->width;
  int h = meta->size->height;
  int s = 0;
  while (w > kTileSize || h > kTileSize) {
    w = dim_at_tile_scale(w, 1);
    h = dim_at_tile_scale(h, 1);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    ++s;
  }
  cov.max_scale = s;
  return cov;
}


std::vector<Store::TileRow> CacheReader::list_tiles(std::string_view uri) const {
  auto m = meta(uri);
  if (!m) return {};
  auto tgt = tile_target(m->content_id);
  if (!tgt) return {};
  return store_->list_tiles_for_region(tgt->media_id, tgt->region_id);
}

std::optional<CacheReader::LqipKey> CacheReader::lqip_key(
    std::string_view uri) const {
  const Store& store = *store_;
  if (auto pdf = parse_pdf_uri(uri)) {
    // Prefer page URI locator, else document file locator.
    if (auto loc = store.find_locator(uri); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, pdf->page};
    const auto fu = file_uri_from_path(pdf->pdf_path);
    if (auto loc = store.find_locator(fu); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, pdf->page};
    return std::nullopt;
  }
  if (auto pimg = parse_pdf_image_uri(uri)) {
    if (auto loc = store.find_locator(uri); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, 0};
    const auto fu = file_uri_from_path(pimg->pdf_path);
    if (auto loc = store.find_locator(fu); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, 0};
    return std::nullopt;
  }
  if (auto dj = parse_djvu_uri(uri)) {
    if (auto loc = store.find_locator(uri); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, dj->page};
    const auto fu = file_uri_from_path(dj->djvu_path);
    if (auto loc = store.find_locator(fu); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, dj->page};
    return std::nullopt;
  }
  if (auto ep = parse_epub_uri(uri)) {
    if (auto loc = store.find_locator(uri); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, ep->page};
    const auto fu = file_uri_from_path(ep->epub_path);
    if (auto loc = store.find_locator(fu); loc && loc->blob_id)
      return LqipKey{*loc->blob_id, ep->page};
    return std::nullopt;
  }
  if (auto loc = store.find_locator(uri); loc && loc->blob_id)
    return LqipKey{*loc->blob_id, 0};
  return std::nullopt;
}

std::optional<std::vector<std::uint8_t>> CacheReader::lqip(
    std::string_view uri) const {
  auto key = lqip_key(uri);
  if (!key) return std::nullopt;
  // Embedded JPEG previews share blob_lqip storage under a distinct kind —
  // never treat them as ThumbHash/Handsum LQIP.
  if (auto kind = store_->get_blob_lqip_kind(key->blob_id, key->page_1based);
      kind && *kind == kLqipKindEmbeddedJpeg) {
    return std::nullopt;
  }
  return store_->get_blob_lqip(key->blob_id, key->page_1based);
}

std::vector<std::string> CacheReader::list_locations(std::string_view prefix,
                                                     std::string_view after,
                                                     int limit) const {
  return store_->list_locator_uris(prefix, after, limit);
}

}  // namespace thumtoo
