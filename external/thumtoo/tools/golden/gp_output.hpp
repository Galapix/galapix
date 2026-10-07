// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Output formats shared by the benchmark tools.
//
//   (default)  human readable: aligned tables, units in the headers, sizes as
//              KiB/MiB, verdicts in words. Meant for a terminal; the layout
//              may change between versions.
//   --csv      one flat table, raw numbers (ms, bytes), RFC 4180 quoting.
//              Stable column names; empty cell = not measured / not applicable.
//   --json     structured documents (gp_json.hpp); schema 1.
//
// stdout carries only the selected format. Warnings and errors go to stderr.

#pragma once

#include "gp_cli.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gp {

// --- mode selection -------------------------------------------------------------

enum class OutputMode { Text, Csv, Json };

/// The options every tool offers for choosing the format.
inline std::vector<cli::Option> output_options() {
  return {
      {"csv", 0, "", "Print one flat CSV table with raw numbers (ms, bytes) instead of "
                     "the human-readable report.", "Output"},
      {"json", 0, "", "Print JSON documents (schema 1) instead of the human-readable "
                      "report.", "Output"},
      {"no-header", 0, "", "With --csv: omit the header row (for appending to an "
                            "existing file).", "Output"},
  };
}

/// Resolve --csv / --json / --no-header; conflicts become usage errors.
inline OutputMode resolve_output_mode(cli::Args& args) {
  const bool csv = args.has("csv");
  const bool json = args.has("json");
  if (csv && json) args.add_error("--csv and --json cannot be combined");
  if (args.has("no-header") && !csv) args.add_error("--no-header only applies to --csv");
  return csv ? OutputMode::Csv : json ? OutputMode::Json : OutputMode::Text;
}

// --- number formatting ----------------------------------------------------------

/// Fixed decimals ("%.*f").
inline std::string fmt_fixed(double v, int digits) {
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.*f", digits, v);
  return buf;
}

/// Milliseconds with ~3 significant digits: 0.008, 1.45, 24.1, 215, 1234.
inline std::string fmt_ms(double ms) {
  if (!std::isfinite(ms)) return "-";
  const double a = std::fabs(ms);
  return fmt_fixed(ms, a < 10 ? 3 : a < 100 ? 2 : a < 1000 ? 1 : 0);
}

/// Binary sizes: 512 B, 183.9 KiB, 49.0 MiB.
inline std::string fmt_bytes(std::uint64_t n) {
  static constexpr const char* kUnits[] = {"B", "KiB", "MiB", "GiB", "TiB"};
  double v = static_cast<double>(n);
  std::size_t u = 0;
  while (v >= 1024.0 && u + 1 < std::size(kUnits)) {
    v /= 1024.0;
    ++u;
  }
  return u == 0 ? std::to_string(n) + " B" : fmt_fixed(v, v < 10 ? 2 : 1) + " " + kUnits[u];
}

/// Cost ratio as "1.39x" (1.00x = best).
inline std::string fmt_ratio(double r) {
  return std::isfinite(r) ? fmt_fixed(r, 2) + "x" : "-";
}

/// Raw machine number for CSV: up to 6 significant digits like the JSON
/// writer; empty for non-finite values.
inline std::string csv_num(double v) {
  if (!std::isfinite(v)) return "";
  std::ostringstream os;
  os << v;
  return os.str();
}

// --- text helpers ---------------------------------------------------------------

/// Prefix every non-empty line of `text` with `indent`.
inline std::string indent_lines(const std::string& text, const std::string& indent) {
  std::string out;
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) out += (line.empty() ? "" : indent) + line + "\n";
  return out;
}

/// Names for the text report: the file name, or the path as given when two
/// inputs share a file name (corpus/a/x.zip, corpus/b/x.zip).
inline std::vector<std::string> display_names(const std::vector<std::filesystem::path>& paths) {
  std::map<std::string, int> count;
  for (const auto& p : paths) ++count[p.filename().string()];
  std::vector<std::string> out;
  for (const auto& p : paths) {
    out.push_back(count[p.filename().string()] > 1 ? p.string() : p.filename().string());
  }
  return out;
}

// --- text table -----------------------------------------------------------------

enum class Align { Left, Right };

struct Column {
  std::string label;  // header text, may carry units: "encode (ms)"
  Align align = Align::Right;
};

/// Column-aligned plain-text table: header, rule, rows. Widths follow the
/// widest cell (counted in UTF-8 code points, so "—" does not skew columns).
class TextTable {
 public:
  explicit TextTable(std::vector<Column> columns) : columns_(std::move(columns)) {}

  void add_row(std::vector<std::string> cells) {
    if (cells.size() != columns_.size()) {
      throw std::logic_error("TextTable: row width does not match header");
    }
    rows_.push_back(std::move(cells));
  }

  bool empty() const { return rows_.empty(); }

  void print(std::ostream& os, const std::string& indent = "  ") const {
    std::vector<std::size_t> width(columns_.size());
    for (std::size_t c = 0; c < columns_.size(); ++c) {
      width[c] = display_width(columns_[c].label);
      for (const auto& row : rows_) width[c] = std::max(width[c], display_width(row[c]));
    }
    print_line(os, indent, width, [&](std::size_t c) -> const std::string& { return columns_[c].label; });
    std::string rule;
    for (std::size_t c = 0; c < columns_.size(); ++c) {
      if (c) rule += "  ";
      rule += std::string(width[c], '-');
    }
    os << indent << rule << '\n';
    for (const auto& row : rows_) {
      print_line(os, indent, width, [&](std::size_t c) -> const std::string& { return row[c]; });
    }
  }

 private:
  static std::size_t display_width(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char ch : s) {
      if ((ch & 0xC0) != 0x80) ++n;  // count UTF-8 lead bytes only
    }
    return n;
  }

  template <typename Cell>
  void print_line(std::ostream& os, const std::string& indent,
                  const std::vector<std::size_t>& width, Cell cell) const {
    std::string line = indent;
    for (std::size_t c = 0; c < columns_.size(); ++c) {
      if (c) line += "  ";
      const std::string& text = cell(c);
      const std::string pad(width[c] - display_width(text), ' ');
      line += columns_[c].align == Align::Right ? pad + text : text + pad;
    }
    // Left-aligned last column would leave trailing blanks.
    while (!line.empty() && line.back() == ' ') line.pop_back();
    os << line << '\n';
  }

  std::vector<Column> columns_;
  std::vector<std::vector<std::string>> rows_;
};

// --- CSV ------------------------------------------------------------------------

/// Quote a field if it contains a comma, quote, CR or LF (RFC 4180).
inline std::string csv_field(std::string_view s) {
  if (s.find_first_of(",\"\r\n") == std::string_view::npos) return std::string(s);
  std::string out = "\"";
  for (char c : s) {
    if (c == '"') out += '"';
    out += c;
  }
  out += '"';
  return out;
}

/// Writes a header once and rows of exactly that width. Lines end in "\n".
class CsvWriter {
 public:
  CsvWriter(std::ostream& os, std::vector<std::string> columns, bool header = true)
      : os_(os), width_(columns.size()) {
    if (header) write(columns);
  }

  void row(const std::vector<std::string>& cells) {
    if (cells.size() != width_) throw std::logic_error("CsvWriter: row width does not match header");
    write(cells);
  }

 private:
  void write(const std::vector<std::string>& cells) {
    for (std::size_t i = 0; i < cells.size(); ++i) {
      if (i) os_ << ',';
      os_ << csv_field(cells[i]);
    }
    os_ << '\n';
  }

  std::ostream& os_;
  std::size_t width_;
};

}  // namespace gp
