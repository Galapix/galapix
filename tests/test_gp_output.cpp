// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gp_output.hpp"

#include <iostream>
#include <limits>
#include <sstream>

namespace {

int g_fails = 0;

void expect_eq(const std::string& got, const std::string& want, const char* msg) {
  if (got != want) {
    std::cerr << "FAIL: " << msg << "\n--- got:\n" << got << "\n--- want:\n" << want << "\n";
    ++g_fails;
  }
}

template <typename F>
void expect_throws(F f, const char* msg) {
  try {
    f();
  } catch (const std::logic_error&) {
    return;
  }
  std::cerr << "FAIL: expected logic_error: " << msg << "\n";
  ++g_fails;
}

}  // namespace

int main() {
  using namespace gp;

  // --- number formatting
  expect_eq(fmt_ms(0.0084), "0.008", "ms <10: 3 decimals");
  expect_eq(fmt_ms(24.13), "24.13", "ms <100: 2 decimals");
  expect_eq(fmt_ms(215.14), "215.1", "ms <1000: 1 decimal");
  expect_eq(fmt_ms(1234.5), "1234", "ms >=1000: integer");  // banker's-safe: .5 -> even
  expect_eq(fmt_ms(std::numeric_limits<double>::quiet_NaN()), "-", "nan ms");
  expect_eq(fmt_bytes(0), "0 B", "0 bytes");
  expect_eq(fmt_bytes(1023), "1023 B", "below 1 KiB");
  expect_eq(fmt_bytes(1024), "1.00 KiB", "1 KiB");
  expect_eq(fmt_bytes(188701), "184.3 KiB", "KiB one decimal >=10");
  expect_eq(fmt_bytes(51411960), "49.0 MiB", "MiB");
  expect_eq(fmt_ratio(1.386), "1.39x", "ratio");
  expect_eq(csv_num(0.000123456789), "0.000123457", "csv_num 6 sig digits");
  expect_eq(csv_num(std::numeric_limits<double>::infinity()), "", "csv_num non-finite");

  // --- CSV quoting / writer
  expect_eq(csv_field("plain"), "plain", "no quoting needed");
  expect_eq(csv_field("a,b"), "\"a,b\"", "comma quoted");
  expect_eq(csv_field("say \"hi\""), "\"say \"\"hi\"\"\"", "quote doubled");
  expect_eq(csv_field("two\nlines"), "\"two\nlines\"", "newline quoted");
  expect_eq(csv_field(""), "", "empty stays empty");
  {
    std::ostringstream os;
    CsvWriter csv(os, {"file", "ms"});
    csv.row({"a b.zip", "1.5"});
    csv.row({"we,ird.zip", ""});
    expect_eq(os.str(), "file,ms\na b.zip,1.5\n\"we,ird.zip\",\n", "csv with header");
    expect_throws([&] { csv.row({"only one"}); }, "csv row width");
    std::ostringstream os2;
    CsvWriter noheader(os2, {"x"}, false);
    noheader.row({"1"});
    expect_eq(os2.str(), "1\n", "csv without header");
  }

  // --- text table
  {
    TextTable t({{"backend", Align::Left}, {"toc (ms)", Align::Right}, {"note", Align::Left}});
    t.add_row({"libarchive", "1.45", "—"});
    t.add_row({"unarr", "0.586", ""});
    std::ostringstream os;
    t.print(os, "  ");
    expect_eq(os.str(),
              "  backend     toc (ms)  note\n"
              "  ----------  --------  ----\n"
              "  libarchive      1.45  —\n"
              "  unarr          0.586\n",
              "aligned table, no trailing blanks, UTF-8 width");
    expect_throws([&] { t.add_row({"too", "short"}); }, "table row width");
  }
  {
    // Multi-byte cell must not skew the next column.
    TextTable t({{"a", Align::Left}, {"b", Align::Right}});
    t.add_row({"—", "1"});
    t.add_row({"abc", "22"});
    std::ostringstream os;
    t.print(os, "");
    expect_eq(os.str(), "a     b\n---  --\n—     1\nabc  22\n", "code point widths");
  }

  // --- text helpers
  expect_eq(indent_lines("a\n\nb\n", "  "), "  a\n\n  b\n", "indent keeps blank lines empty");
  {
    const std::vector<std::filesystem::path> p = {"c/a/x.zip", "c/b/x.zip", "c/y.zip"};
    const auto names = display_names(p);
    expect_eq(names[0] + "|" + names[1] + "|" + names[2], "c/a/x.zip|c/b/x.zip|y.zip",
              "shared file names fall back to the path");
  }

  // --- output mode resolution
  {
    cli::Args a;
    a.add_option("csv", "");
    a.add_option("json", "");
    resolve_output_mode(a);
    expect_eq(a.errors().empty() ? "no error" : a.errors()[0], "--csv and --json cannot be combined",
              "csv+json conflict");
    cli::Args b;
    b.add_option("no-header", "");
    resolve_output_mode(b);
    expect_eq(b.errors().empty() ? "no error" : b.errors()[0], "--no-header only applies to --csv",
              "no-header without csv");
    cli::Args c;
    c.add_option("csv", "");
    c.add_option("no-header", "");
    expect_eq(resolve_output_mode(c) == OutputMode::Csv && c.errors().empty() ? "ok" : "bad", "ok",
              "csv + no-header");
    cli::Args d;
    expect_eq(resolve_output_mode(d) == OutputMode::Text ? "ok" : "bad", "ok", "default is text");
  }

  if (g_fails) {
    std::cerr << g_fails << " failure(s)\n";
    return 1;
  }
  std::cout << "ok: gp_output\n";
  return 0;
}
