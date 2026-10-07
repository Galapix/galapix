// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Session soft pixels vs tiles (Store-only; no durable schema-4 levels).
///
/// Contract:
/// * kMaxSoftLadderEdge (512) caps request_pixels session replies.
/// * request_pixels(2048) must not return a full-page decode above that cap.
/// * An existing 256 soft reply must not block upgrade toward 512.
/// * PDF page high-res remains request_tile (see test_pdf_tiles.cpp).

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/uri.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

void expect_le(int a, int b, const char* msg) {
  if (a > b) {
    std::cerr << "FAIL: " << msg << " (" << a << " > " << b << ")\n";
    ++g_failures;
  }
}

void expect_ge(int a, int b, const char* msg) {
  if (a < b) {
    std::cerr << "FAIL: " << msg << " (" << a << " < " << b << ")\n";
    ++g_failures;
  }
}

std::optional<fs::path> make_test_pdf(const fs::path& dir) {
  std::string body;
  for (int i = 0; i < 40; ++i) {
    body += "BT /F1 10 Tf 36 ";
    body += std::to_string(750 - i * 18);
    body += " Td (Line ";
    body += std::to_string(i);
    body += " ABCDEFGHIJKLMNOPQRSTUVWXYZ012345) Tj ET\n";
  }
  std::string pdf;
  std::vector<std::size_t> obj_off(6, 0);
  auto append = [&](std::string_view s) { pdf.append(s); };
  append("%PDF-1.4\n");
  obj_off[1] = pdf.size();
  append("1 0 obj<< /Type /Catalog /Pages 2 0 R >>endobj\n");
  obj_off[2] = pdf.size();
  append("2 0 obj<< /Type /Pages /Kids [3 0 R] /Count 1 >>endobj\n");
  obj_off[3] = pdf.size();
  append(
      "3 0 obj<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
      "/Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>endobj\n");
  obj_off[4] = pdf.size();
  {
    std::string stream = body;
    append("4 0 obj<< /Length ");
    append(std::to_string(stream.size()));
    append(" >>stream\n");
    append(stream);
    append("\nendstream\nendobj\n");
  }
  obj_off[5] = pdf.size();
  append(
      "5 0 obj<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>endobj\n");
  const std::size_t xref = pdf.size();
  append("xref\n0 6\n0000000000 65535 f \n");
  for (int i = 1; i <= 5; ++i) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%010zu 00000 n \n", obj_off[i]);
    append(buf);
  }
  append("trailer<< /Size 6 /Root 1 0 R >>\nstartxref\n");
  append(std::to_string(xref));
  append("\n%%EOF\n");

  fs::create_directories(dir);
  const fs::path path = dir / "soft_ladder.pdf";
  std::ofstream out(path, std::ios::binary);
  if (!out) return std::nullopt;
  out.write(pdf.data(), static_cast<std::streamsize>(pdf.size()));
  return path;
}

std::optional<thumtoo::PixelLevel> wait_pixels(thumtoo::Client& c,
                                               const std::string& uri,
                                               int edge, int timeout_ms = 30000) {
  std::mutex mu;
  std::optional<thumtoo::PixelLevel> got;
  bool done = false;
  c.request_pixels(uri, edge, [&](std::string, int, std::optional<thumtoo::PixelLevel> px) {
    std::lock_guard lock(mu);
    got = std::move(px);
    done = true;
  });
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    {
      std::lock_guard lock(mu);
      if (done) return got;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return std::nullopt;
}

}  // namespace

int main() {
  // Durable soft ladder behaviour under test; tiles-first default skips put_level.
  // Session soft replies under Store-only (no durable legacy levels).
  unsetenv("THUMTOO_SOFT_LEVELS");
  unsetenv("THUMTOO_STORE_ONLY");
  const fs::path tmp =
      fs::temp_directory_path() / "thumtoo-test-soft-ladder";
  fs::remove_all(tmp);
  fs::create_directories(tmp);
  const auto pdf = make_test_pdf(tmp);
  expect(pdf.has_value(), "make_test_pdf");
  if (!pdf) return 1;

  const fs::path cache = tmp / "cache";
  auto client = thumtoo::Client::open(cache, {}, 0, cache);
  if (!client) {
    std::cerr << "FAIL: Client::open\n";
    return 1;
  }
  const std::string uri = thumtoo::pdf_page_uri(*pdf, 1);

  // 1) First soft step (256).
  auto px256 = wait_pixels(*client, uri, 256);
  expect(px256.has_value() && !px256->bytes.empty(), "pixels@256 non-empty");
  if (px256) {
    expect_le(std::max(px256->width, px256->height), 256, "256 long edge ≤ 256");
    expect_ge(std::max(px256->width, px256->height), 200, "256 long edge ≥ 200");
  }

  // 2) Upgrade must not stick on 256 when asking for 512.
  auto px512 = wait_pixels(*client, uri, 512);
  expect(px512.has_value() && !px512->bytes.empty(), "pixels@512 non-empty");
  if (px512) {
    const int long_px = std::max(px512->width, px512->height);
    expect_ge(long_px, (512 * 9) / 10, "512 upgrade long edge ≥ ~512");
    expect_le(long_px, 512, "512 long edge ≤ 512");
  }

  // 3) Request above soft max → still only soft (≤ kMaxSoftLadderEdge).
  auto px2k = wait_pixels(*client, uri, 2048);
  expect(px2k.has_value() && !px2k->bytes.empty(), "pixels@2048 non-empty");
  if (px2k) {
    const int long_px = std::max(px2k->width, px2k->height);
    expect_le(long_px, thumtoo::kMaxSoftLadderEdge,
              "2048 request still soft-capped");
    expect_ge(long_px, (thumtoo::kMaxSoftLadderEdge * 9) / 10,
              "2048 request returns full soft max");
  }

  // 4) Cache-only get_pixels: under tiles-first Store-only there is no durable
  // soft level — empty is OK. If present, must stay soft-capped.
  if (auto cached = client->get_pixels(uri, 2048)) {
    expect_le(std::max(cached->width, cached->height),
              thumtoo::kMaxSoftLadderEdge, "get_pixels(2048) soft-capped");
  }

  // 5) Sanity: tile path still available for high-res (one cell).
  {
    std::mutex mu;
    bool done = false;
    std::optional<thumtoo::TileBlob> tile;
    client->request_tile(uri, /*scale=*/0, 0, 0,
                        [&](std::string, int, int, int,
                            std::optional<thumtoo::TileBlob> t) {
                          std::lock_guard lock(mu);
                          tile = std::move(t);
                          done = true;
                        });
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (std::chrono::steady_clock::now() < deadline) {
      std::lock_guard lock(mu);
      if (done) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    expect(done && tile && !tile->bytes.empty(), "request_tile scale0 delivers");
  }

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "OK test_soft_ladder\n";
  return 0;
}
