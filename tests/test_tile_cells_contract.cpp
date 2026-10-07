// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * request_tile_cells contract (TILES.md "Interactive cell contract"):
 * every requested cell gets exactly one TileResult — Ok, Cancelled, Failed or
 * Unavailable — including under cancel, interest-epoch bumps and shutdown.
 *
 * Regression: thumtoo used to drop cancelled tile jobs without any callback
 * (reply_cancelled_job) and bump_interest_epoch purged queued tile jobs the
 * same way. Hosts kept those cells InFlight forever ("stuck on low res").
 */

#include "thumtoo/client.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/image.hpp"
#include "thumtoo/uri.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;
void expect(bool c, const std::string& m) {
  if (!c) {
    std::cerr << "FAIL: " << m << "\n";
    ++g_failures;
  }
}

void write_bmp(const fs::path& path, int w, int h) {
  const int row_raw = w * 3;
  const int row_pad = (4 - (row_raw % 4)) % 4;
  const int row_stride = row_raw + row_pad;
  const int pixel_size = row_stride * h;
  const int file_size = 14 + 40 + pixel_size;
  std::vector<unsigned char> buf(static_cast<size_t>(file_size), 0);
  auto put16 = [&](int off, int v) {
    buf[static_cast<size_t>(off)] = static_cast<unsigned char>(v & 0xff);
    buf[static_cast<size_t>(off + 1)] = static_cast<unsigned char>((v >> 8) & 0xff);
  };
  auto put32 = [&](int off, int v) {
    put16(off, v & 0xffff);
    put16(off + 2, (v >> 16) & 0xffff);
  };
  buf[0] = 'B';
  buf[1] = 'M';
  put32(2, file_size);
  put32(10, 54);
  put32(14, 40);
  put32(18, w);
  put32(22, h);
  put16(26, 1);
  put16(28, 24);
  put32(34, pixel_size);
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      const int i = 54 + y * row_stride + x * 3;
      buf[static_cast<size_t>(i) + 0] = static_cast<unsigned char>(x);
      buf[static_cast<size_t>(i) + 1] = static_cast<unsigned char>(y);
      buf[static_cast<size_t>(i) + 2] = 200;
    }
  }
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(buf.data()),
            static_cast<std::streamsize>(buf.size()));
}

/// Letter page fully covered by one 256x256 JPEG (a 30 dpi raster page).
bool write_scan_pdf(const fs::path& path) {
  std::vector<std::uint8_t> rgb(256 * 256 * 3, 128);
  for (std::size_t i = 0; i < rgb.size(); ++i) {
    rgb[i] = static_cast<std::uint8_t>(i * 7);
  }
  auto jpeg = thumtoo::encode_tile_cell_rgb(rgb.data(), 256, 256, 0, 0, 0, 85);
  if (!jpeg || jpeg->bytes.empty()) {
    return false;
  }
  std::vector<std::string> objs;
  objs.push_back("<< /Type /Catalog /Pages 2 0 R >>");
  objs.push_back("<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
  objs.push_back("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
                 "/Resources << /XObject << /Im0 4 0 R >> >> /Contents 5 0 R >>");
  objs.push_back("<< /Type /XObject /Subtype /Image /Width 256 /Height 256 "
                 "/ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode "
                 "/Length " + std::to_string(jpeg->bytes.size()) + " >>\nstream\n" +
                 std::string(jpeg->bytes.begin(), jpeg->bytes.end()) + "\nendstream");
  const std::string content = "q 612 0 0 792 0 0 cm /Im0 Do Q";
  objs.push_back("<< /Length " + std::to_string(content.size()) + " >>\nstream\n" +
                 content + "\nendstream");
  std::string data = "%PDF-1.4\n";
  std::vector<std::size_t> offs;
  for (std::size_t i = 0; i < objs.size(); ++i) {
    offs.push_back(data.size());
    data += std::to_string(i + 1) + " 0 obj\n" + objs[i] + "\nendobj\n";
  }
  const std::size_t xref = data.size();
  data += "xref\n0 " + std::to_string(objs.size() + 1) + "\n0000000000 65535 f \n";
  for (std::size_t o : offs) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%010zu 00000 n \n", o);
    data += buf;
  }
  data += "trailer\n<< /Size " + std::to_string(objs.size() + 1) +
          " /Root 1 0 R >>\nstartxref\n" + std::to_string(xref) + "\n%%EOF\n";
  std::ofstream out(path, std::ios::binary);
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
  return static_cast<bool>(out);
}

/// Collects results per (batch, index) and checks exactly-once delivery.
struct Recorder {
  std::mutex mu;
  std::map<std::pair<int, std::size_t>, std::vector<thumtoo::TileResult>> got;

  thumtoo::Client::TileResultCallback for_batch(int batch) {
    return [this, batch](std::size_t index, thumtoo::TileResult r) {
      std::lock_guard lock(mu);
      got[{batch, index}].push_back(std::move(r));
    };
  }

  std::size_t delivered() {
    std::lock_guard lock(mu);
    return got.size();
  }

  void expect_exactly_once(int batch, std::size_t n, const std::string& what) {
    std::lock_guard lock(mu);
    for (std::size_t i = 0; i < n; ++i) {
      auto it = got.find({batch, i});
      const std::size_t count = it == got.end() ? 0 : it->second.size();
      expect(count == 1, what + ": cell " + std::to_string(i) +
                             " delivered " + std::to_string(count) +
                             " times (want exactly 1)");
    }
  }

  thumtoo::TileResult result(int batch, std::size_t index) {
    std::lock_guard lock(mu);
    auto it = got.find({batch, index});
    if (it == got.end() || it->second.empty()) {
      return {};
    }
    return it->second.front();
  }
};

void settle(thumtoo::Client& client, Recorder& rec, std::size_t want) {
  for (int i = 0; i < 400 && rec.delivered() < want; ++i) {
    client.drain();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
}

}  // namespace

int main() {
  using namespace thumtoo;
  image_library_init();
  const auto root =
      fs::temp_directory_path() / "thumtoo-tile-cells-contract" /
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  fs::create_directories(root);
  const auto img = root / "grid.bmp";
  constexpr int W = 600;  // scale 0: 3×2 cells
  constexpr int H = 400;
  write_bmp(img, W, H);
  const auto uri = file_uri_from_path(img);
  const auto cache = root / "cache";

  // 1. Classification: Ok / Unavailable, one result per cell.
  {
    auto client = Client::open(cache, {}, /*worker_threads=*/2, cache);
    expect(!!client, "open client");
    Recorder rec;
    std::vector<Client::TileCoord> coords{
        {0, 0, 0}, {0, 2, 1}, {0, 9, 9}, {-1, 0, 0}, {5, 0, 0}};
    client->request_tile_cells(uri, coords, rec.for_batch(0));
    settle(*client, rec, coords.size());
    rec.expect_exactly_once(0, coords.size(), "classification");
    auto r0 = rec.result(0, 0);
    expect(r0.status == TileStatus::Ok && r0.tile && !r0.tile->bytes.empty(),
           "cell (0,0,0) Ok with pixels");
    expect(rec.result(0, 1).status == TileStatus::Ok, "edge cell (0,2,1) Ok");
    auto out = rec.result(0, 2);
    expect(out.status == TileStatus::Unavailable,
           std::string("out-of-grid cell is Unavailable, got ") +
               tile_status_name(out.status) + " (" + out.error + ")");
    expect(!out.error.empty(), "out-of-grid cell carries a reason");
    auto neg = rec.result(0, 3);
    expect(neg.status == TileStatus::Unavailable,
           "negative scale on raster is Unavailable");
    expect(rec.result(0, 4).status == TileStatus::Ok, "coarse single cell Ok");
  }

  // 2. Missing source → Failed with a reason (not silence, not Ok).
  {
    auto client = Client::open(cache, {}, 1, cache);
    Recorder rec;
    const auto missing = file_uri_from_path(root / "does-not-exist.jpg");
    client->request_tile_cells(missing, {{0, 0, 0}, {1, 0, 0}}, rec.for_batch(0));
    settle(*client, rec, 2);
    rec.expect_exactly_once(0, 2, "missing source");
    auto r = rec.result(0, 0);
    expect(r.status == TileStatus::Failed, "missing source is Failed");
    expect(!r.error.empty(), "missing source carries a reason");
  }

  // 3. Cancel + interest-epoch bump under load: still exactly once per cell.
  {
    auto client = Client::open(cache, {}, 1, cache);
    Recorder rec;
    constexpr int kBatches = 24;
    std::vector<std::vector<Client::TileCoord>> all;
    for (int b = 0; b < kBatches; ++b) {
      std::vector<Client::TileCoord> c{{0, 0, 0}, {0, 1, 0}, {0, 2, 0},
                                       {0, 0, 1}, {0, 1, 1}, {0, 2, 1}};
      client->request_tile_cells(uri, c, rec.for_batch(b));
      all.push_back(c);
    }
    // set_interest-style epoch bump must not drop host-owned cell jobs.
    (void)client->bump_interest_epoch();
    const std::vector<Client::TileCoord> cancel{{0, 1, 0}, {0, 2, 1}};
    const std::size_t cancelled = client->cancel_tile_cells(uri, cancel);
    expect(cancelled > 0, "cancel_tile_cells reached queued cells");
    settle(*client, rec, kBatches * 6);
    int n_cancelled = 0;
    for (int b = 0; b < kBatches; ++b) {
      rec.expect_exactly_once(b, 6, "cancel under load batch " + std::to_string(b));
      for (std::size_t i = 0; i < 6; ++i) {
        const auto r = rec.result(b, i);
        if (r.status == TileStatus::Cancelled) {
          ++n_cancelled;
          expect(!r.error.empty(), "Cancelled carries a reason");
          const auto& c = all[static_cast<size_t>(b)][i];
          expect((c.x == 1 && c.y == 0) || (c.x == 2 && c.y == 1),
                 "only requested cells are cancelled");
        }
      }
    }
    expect(n_cancelled > 0, "some cells reported Cancelled");
  }

  // 4. Shutdown with a backlog: every queued cell still gets an answer.
  {
    Recorder rec;
    constexpr int kBatches = 16;
    {
      auto client = Client::open(cache, {}, 1, cache);
      for (int b = 0; b < kBatches; ++b) {
        client->request_tile_cells(uri, {{0, 0, 0}, {0, 1, 1}}, rec.for_batch(b));
      }
    }  // ~Client: queued jobs are cancelled, running job finishes
    for (int b = 0; b < kBatches; ++b) {
      rec.expect_exactly_once(b, 2, "shutdown batch " + std::to_string(b));
    }
  }

  // 5. Legacy request_tiles wrapper still answers Ok cells.
  {
    auto client = Client::open(cache, {}, 1, cache);
    std::mutex mu;
    int ok = 0;
    client->request_tiles(uri, {{0, 0, 0}, {0, 1, 0}},
                          [&](std::size_t, std::optional<TileBlob> t) {
                            std::lock_guard lock(mu);
                            ok += t ? 1 : 0;
                          });
    for (int i = 0; i < 400; ++i) {
      client->drain();
      {
        std::lock_guard lock(mu);
        if (ok == 2) break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    expect(ok == 2, "legacy request_tiles delivers both cells");
  }

  // 6. Raster-only PDF page: scales finer than the image's native
  //    resolution are Unavailable with the reason from the page profile (not
  //    a generic Failed that hosts retry); the layout scale renders.
#if defined(THUMTOO_HAVE_MUPDF)
  {
    const auto pdf = root / "scan.pdf";
    expect(write_scan_pdf(pdf), "write scan pdf");
    auto client = Client::open(cache, {}, 2, cache);
    Recorder rec;
    const std::string page = file_uri_from_path(pdf) + "//page:1";
    // 256 px across 8.5 in = 30 dpi: the finest useful scale is 0 (144 dpi).
    client->request_tile_cells(page, {{0, 1, 1}, {-1, 2, 2}}, rec.for_batch(0));
    settle(*client, rec, 2);
    rec.expect_exactly_once(0, 2, "raster page cap");
    auto ok = rec.result(0, 0);
    expect(ok.status == TileStatus::Ok,
           std::string("s=0 renders, got ") + tile_status_name(ok.status) +
               " (" + ok.error + ")");
    auto fine = rec.result(0, 1);
    expect(fine.status == TileStatus::Unavailable,
           std::string("s=-1 is Unavailable, got ") + tile_status_name(fine.status) +
               " (" + fine.error + ")");
    expect(fine.error.find("finest useful scale 0") != std::string::npos,
           "refusal names the cap: " + fine.error);
  }
#endif

  thumtoo::pdf_release_document_cache();
  fs::remove_all(root);
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "tile_cells_contract OK\n";
  return 0;
}
