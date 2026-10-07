// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Golden-path JPEG decode microbench (vips only — no thumtoo Client/Store).
// Measures the size header, a full load, jpegload with shrink-on-load 2/4/8
// and vips_thumbnail to two edge lengths. See --help.
//
// Every operation is verified once before it is timed: a decode that fails
// (not a JPEG, truncated file, ...) marks the file as failed instead of
// contributing a near-zero "timing".

#include <vips/vips.h>

#include "golden/gp_cli.hpp"
#include "golden/gp_common.hpp"
#include "golden/gp_json.hpp"
#include "golden/gp_output.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

void ensure_vips() {
  static bool once = false;
  if (!once) {
    if (VIPS_INIT("microbench_decode")) {
      std::cerr << "VIPS_INIT failed\n";
      std::exit(1);
    }
    once = true;
  }
}

/// First line of the vips error buffer (then clears it).
std::string take_vips_error() {
  std::string s = vips_error_buffer();
  vips_error_clear();
  if (auto nl = s.find('\n'); nl != std::string::npos) s.resize(nl);
  return s.empty() ? "unknown libvips error" : s;
}

bool is_jpeg(const fs::path& p) {
  ensure_vips();
  const char* loader = vips_foreign_find_load(p.string().c_str());
  if (!loader) {
    vips_error_clear();
    return false;
  }
  return std::string(loader).rfind("VipsForeignLoadJpeg", 0) == 0;
}

// --- operations -----------------------------------------------------------------

/// One timed operation. `run` returns false if libvips reported an error.
struct Op {
  const char* json_key;  // "size_ms": stable key for JSON / CSV
  const char* label;     // text header: "size"
  const char* what;      // for failure messages
  bool heavy;            // full decodes get half the repeats
  std::function<bool(const std::string&)> run;
};

bool drain(VipsImage* img) {
  if (!img) return false;
  size_t len = 0;
  void* buf = vips_image_write_to_memory(img, &len);
  const bool ok = buf != nullptr;
  if (buf) g_free(buf);
  g_object_unref(img);
  return ok;
}

const std::vector<Op>& operations() {
  static const std::vector<Op> ops = [] {
    std::vector<Op> v;
    v.push_back({"size_ms", "size", "reading the image size", false, [](const std::string& p) {
                   VipsImage* img = vips_image_new_from_file(
                       p.c_str(), "access", VIPS_ACCESS_SEQUENTIAL, nullptr);
                   if (!img) return false;
                   (void)vips_image_get_width(img);
                   (void)vips_image_get_height(img);
                   g_object_unref(img);
                   return true;
                 }});
    v.push_back({"full_ms", "full", "a full decode", true, [](const std::string& p) {
                   return drain(vips_image_new_from_file(p.c_str(), nullptr));
                 }});
    static const char* const keys[] = {"shrink2_ms", "shrink4_ms", "shrink8_ms"};
    static const char* const labels[] = {"shrink2", "shrink4", "shrink8"};
    static const int shrinks[] = {2, 4, 8};
    for (int i = 0; i < 3; ++i) {
      const int shrink = shrinks[i];
      v.push_back({keys[i], labels[i], "a shrink-on-load decode", false,
                   [shrink](const std::string& p) {
                     VipsImage* img = nullptr;
                     if (vips_jpegload(p.c_str(), &img, "shrink", shrink, nullptr) != 0) {
                       return false;
                     }
                     return drain(img);
                   }});
    }
    for (int edge : {32, 256}) {
      v.push_back({edge == 32 ? "thumb32_ms" : "thumb256_ms", edge == 32 ? "thumb32" : "thumb256",
                   "a thumbnail", false, [edge](const std::string& p) {
                     VipsImage* thumb = nullptr;
                     if (vips_thumbnail(p.c_str(), &thumb, edge, "size", VIPS_SIZE_DOWN,
                                        nullptr) != 0) {
                       return false;
                     }
                     return drain(thumb);
                   }});
    }
    return v;
  }();
  return ops;
}

// --- one file -------------------------------------------------------------------

struct Case {
  fs::path path;
  std::string name;   // display name
  std::string error;  // non-empty: the file could not be measured
  int width = 0, height = 0;
  double mpix = 0;
  std::vector<double> median_ms;  // aligned with operations()
  bool ok() const { return error.empty(); }
};

void measure_case(Case& c, int repeats) {
  gp::StderrSilencer quiet;  // libvips/libjpeg warnings repeat on every call
  ensure_vips();
  const std::string path = c.path.string();
  if (!is_jpeg(c.path)) {
    c.error = "not a JPEG image";
    return;
  }
  VipsImage* hdr =
      vips_image_new_from_file(path.c_str(), "access", VIPS_ACCESS_SEQUENTIAL, nullptr);
  if (!hdr) {
    c.error = "open failed: " + take_vips_error();
    return;
  }
  c.width = vips_image_get_width(hdr);
  c.height = vips_image_get_height(hdr);
  g_object_unref(hdr);
  c.mpix = static_cast<double>(c.width) * static_cast<double>(c.height) / 1e6;

  // libvips quietly pads a truncated JPEG with gray; timing that would measure
  // a decode of half an image. Insist on a complete file.
  {
    VipsImage* whole = nullptr;
    if (vips_jpegload(path.c_str(), &whole, "fail_on", VIPS_FAIL_ON_TRUNCATED, nullptr) != 0 ||
        !drain(whole)) {
      c.error = "damaged or truncated JPEG: " + take_vips_error();
      c.median_ms.clear();
      return;
    }
  }

  for (const Op& op : operations()) {
    if (!op.run(path)) {  // verify once; a failing op is never timed
      c.error = std::string(op.what) + " failed: " + take_vips_error();
      c.median_ms.clear();
      return;
    }
    const int reps = op.heavy ? std::max(1, repeats / 2) : repeats;
    c.median_ms.push_back(gp::time_median(reps, [&] { (void)op.run(path); }).median);
  }
}

// --- output ---------------------------------------------------------------------

/// Key order and names are kept stable for checked-in baselines; "status" is
/// additive (and "reason" replaces width/height/metrics on failure).
void write_case_json(gp::JsonWriter& w, const Case& c) {
  w.begin_object();
  w.field("path", c.path.string());
  w.field("file", c.path.filename().string());
  if (!c.ok()) {
    w.field("status", "failed");
    w.field("reason", c.error);
    w.end_object();
    return;
  }
  w.field("status", "ok");
  w.field("width", c.width);
  w.field("height", c.height);
  w.field("mpix", c.mpix);
  w.key("metrics").begin_object();
  for (std::size_t i = 0; i < operations().size(); ++i) {
    w.field(operations()[i].json_key, c.median_ms[i]);
  }
  w.end_object();
  w.end_object();
}

std::vector<std::string> csv_columns() {
  std::vector<std::string> cols = {"file", "width", "height", "mpix", "status", "reason"};
  for (const Op& op : operations()) cols.push_back(op.json_key);
  return cols;
}

std::vector<std::string> csv_row(const Case& c) {
  std::vector<std::string> row = {c.path.string(),
                                  c.ok() ? std::to_string(c.width) : "",
                                  c.ok() ? std::to_string(c.height) : "",
                                  c.ok() ? gp::csv_num(c.mpix) : "",
                                  c.ok() ? "ok" : "failed",
                                  c.error};
  for (std::size_t i = 0; i < operations().size(); ++i) {
    row.push_back(c.ok() ? gp::csv_num(c.median_ms[i]) : "");
  }
  return row;
}

gp::TextTable text_table() {
  std::vector<gp::Column> cols = {{"file", gp::Align::Left}, {"MP", gp::Align::Right}};
  for (const Op& op : operations()) cols.push_back({op.label, gp::Align::Right});
  return gp::TextTable(std::move(cols));
}

gp::cli::Spec make_spec() {
  gp::cli::Spec s;
  s.program = "thumtoo-microbench-decode";
  s.synopsis = "[OPTION]... FILE.jpg|DIR...";
  s.summary =
      "Time how fast libvips can get what a thumbnailer needs out of a JPEG. Each "
      "image is measured on its own, through libvips only (no thumtoo cache or "
      "database), so the numbers are a floor for what thumtoo itself can achieve.\n\n"
      "Timings are medians of --repeat runs after one warm-up on a warm page cache, "
      "in milliseconds. Each operation is run once and checked before it is timed; a "
      "file that cannot be decoded is reported as failed rather than as a fast one.";
  s.options = {
      {"repeat", 'n', "N",
       "Timed runs per measurement, 1-1000 (default 5). The full decode gets half."},
      {"recursive", 'r', "",
       "Search subdirectories of any DIR as well. Directories are searched for JPEG "
       "files by content, sorted by name."},
  };
  for (const auto& o : gp::output_options()) s.options.push_back(o);
  s.sections = {
      {"Columns",
       "MP            image size in megapixels\n"
       "size          open the file and read width and height only\n"
       "full          decode the whole image into memory\n"
       "shrink2/4/8   jpegload with shrink-on-load: the JPEG is decoded already\n"
       "              scaled down by 2, 4 or 8 (DCT domain, far cheaper than full)\n"
       "thumb32/256   vips_thumbnail to that long edge (uses shrink-on-load)\n"
       "In CSV and JSON the same columns are named size_ms, full_ms, shrink2_ms, ..."},
      {"Output formats",
       "Default: an aligned table, one row per file.\n"
       "--csv:   one row per file; raw milliseconds; status/reason columns.\n"
       "--json:  one document {schema, tool, repeats, warmups, cases: [...]}.\n"
       "Warnings and errors go to stderr, so stdout is clean for pipes."},
      {"Exit status",
       "0  every file was measured\n"
       "1  a file could not be decoded\n"
       "2  usage error"},
      {"Examples",
       "thumtoo-microbench-decode photo.jpg\n"
       "thumtoo-microbench-decode -n 10 corpus/synthetic/jpeg/\n"
       "thumtoo-microbench-decode --csv -r ~/Pictures > decode.csv\n"
       "thumtoo-microbench-decode --json a.jpg b.jpg"},
  };
  return s;
}

}  // namespace

int main(int argc, char** argv) {
  namespace cli = gp::cli;
  const cli::Spec spec = make_spec();
  cli::Args args;
  switch (cli::parse(spec, argc, argv, args)) {
    case cli::Parsed::Help:
      cli::print_help(std::cout, spec);
      return 0;
    case cli::Parsed::Version:
      cli::print_version(std::cout, spec.program);
      return 0;
    case cli::Parsed::Error:
      return cli::fail_usage(spec, args.errors());
    case cli::Parsed::Run:
      break;
  }

  const int repeats = args.get_int("repeat", 5, 1, 1000);
  const gp::OutputMode mode = gp::resolve_output_mode(args);
  if (args.positionals().empty()) args.add_error("missing FILE argument");
  std::vector<std::string> input_errors;
  const std::vector<fs::path> inputs =
      cli::expand_inputs(args.positionals(), args.has("recursive"), is_jpeg, input_errors);
  for (const std::string& e : input_errors) args.add_error(e);
  if (args.errors().empty() && inputs.empty()) args.add_error("no JPEG files to measure");
  if (!args.errors().empty()) return cli::fail_usage(spec, args.errors());

  const std::vector<std::string> names = gp::display_names(inputs);
  gp::TextTable table = text_table();
  std::vector<std::string> failures;
  std::optional<gp::CsvWriter> csv;
  std::optional<gp::JsonWriter> json;
  if (mode == gp::OutputMode::Csv) {
    csv.emplace(std::cout, csv_columns(), !args.has("no-header"));
  } else if (mode == gp::OutputMode::Json) {
    json.emplace(std::cout);
    json->begin_object();
    json->field("schema", 1);
    json->field("tool", "thumtoo-microbench-decode");
    json->field("repeats", repeats);
    json->field("warmups", 1);
    json->key("cases").begin_array();
  }

  cli::Progress progress;
  std::size_t failed = 0;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    progress.update(i + 1, inputs.size(), names[i]);
    Case c;
    c.path = inputs[i];
    c.name = names[i];
    measure_case(c, repeats);
    progress.clear();
    if (!c.ok()) {
      ++failed;
      failures.push_back(c.name + ": failed — " + c.error);
    }
    switch (mode) {
      case gp::OutputMode::Text:
        if (c.ok()) {
          std::vector<std::string> row = {c.name, gp::fmt_fixed(c.mpix, 2)};
          for (double ms : c.median_ms) row.push_back(gp::fmt_ms(ms));
          table.add_row(std::move(row));
        }
        break;
      case gp::OutputMode::Csv:
        csv->row(csv_row(c));
        break;
      case gp::OutputMode::Json:
        write_case_json(*json, c);
        break;
    }
  }

  if (mode == gp::OutputMode::Text) {
    if (!table.empty()) {
      std::cout << "Times in milliseconds, median of " << repeats << " runs.\n";
      table.print(std::cout, "");
    }
    for (const std::string& f : failures) std::cout << f << "\n";
  } else if (mode == gp::OutputMode::Json) {
    json->end_array();
    json->end_object();
  }
  if (failed > 0) {
    std::cerr << "thumtoo-microbench-decode: " << failed << " of " << inputs.size()
              << (inputs.size() == 1 ? " file" : " files") << " could not be decoded\n";
    return 1;
  }
  return 0;
}
