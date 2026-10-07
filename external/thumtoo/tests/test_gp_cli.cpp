// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gp_cli.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace {

namespace cli = gp::cli;
namespace fs = std::filesystem;

int g_fails = 0;

void expect(bool cond, const std::string& msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_fails;
  }
}

cli::Spec make_spec() {
  cli::Spec s;
  s.program = "prog";
  s.synopsis = "[OPTION]... FILE...";
  s.summary = "First paragraph that is long enough that it has to be wrapped onto "
              "more than one line when printed in the help output.\n\nSecond paragraph.";
  s.options = {
      {"repeat", 'n', "N", "Timed runs per measurement (default 5)."},
      {"json", 0, "", "Machine-readable JSON.", "Output"},
      {"tie-pct", 0, "P", "Ties are results within P percent of the best.", "Output"},
      {"a-rather-long-option-name", 0, "SOMETHING", "Long left column goes on its own line."},
      {"recursive", 'r', "", "Search directories recursively."},
      {"max-scale", 0, "M", "May be negative."},
  };
  s.sections = {{"Examples", "prog a.zip\nprog --json dir/\n\nprog -n 3 b.zip"}};
  return s;
}

struct Run {
  cli::Parsed parsed;
  cli::Args args;
};

Run run(std::vector<std::string> argv_list) {
  std::vector<char*> argv;
  std::string prog = "prog";
  argv.push_back(prog.data());
  for (auto& a : argv_list) argv.push_back(a.data());
  Run r{cli::Parsed::Run, {}};
  r.parsed = cli::parse(make_spec(), static_cast<int>(argv.size()), argv.data(), r.args);
  return r;
}

bool has_error(const Run& r, const std::string& needle) {
  for (const auto& e : r.args.errors()) {
    if (e.find(needle) != std::string::npos) return true;
  }
  return false;
}

}  // namespace

int main() {
  // --- syntax variants
  {
    auto r = run({"--repeat", "7", "a.zip", "--json", "-r", "b.zip"});
    expect(r.parsed == cli::Parsed::Run, "plain parse ok");
    expect(r.args.get_int("repeat", 5, 1, 100) == 7, "--repeat 7");
    expect(r.args.has("json") && r.args.has("recursive"), "flags set");
    expect(r.args.positionals().size() == 2, "positionals around options");
  }
  {
    auto r = run({"--repeat=9", "-n", "3", "-n4"});
    expect(r.args.get_int("repeat", 5, 1, 100) == 4, "last value wins (-n4)");
    expect(r.args.values("repeat").size() == 3, "all values kept");
  }
  {
    auto r = run({"--max-scale", "-1", "--", "--json", "-x"});
    expect(r.parsed == cli::Parsed::Run, "value may start with '-'");
    expect(r.args.get_int("max-scale", 0, -10, 10) == -1, "negative value");
    expect(!r.args.has("json"), "-- ends options");
    expect(r.args.positionals().size() == 2, "positionals after --");
  }
  {
    auto r = run({"-", "a"});
    expect(r.args.positionals().size() == 2, "lone - is a positional");
  }

  // --- errors
  {
    auto r = run({"--repat", "3"});
    expect(r.parsed == cli::Parsed::Error, "unknown option is an error");
    expect(has_error(r, "unknown option '--repat'") && has_error(r, "did you mean '--repeat'"),
           "typo suggestion");
  }
  {
    auto r = run({"--zzzzzzzz"});
    expect(r.parsed == cli::Parsed::Error && !has_error(r, "did you mean"), "no wild guess");
  }
  {
    auto r = run({"--repeat"});
    expect(r.parsed == cli::Parsed::Error && has_error(r, "requires a value"),
           "missing value");
  }
  {
    auto r = run({"--json=1"});
    expect(r.parsed == cli::Parsed::Error && has_error(r, "does not take a value"),
           "flag with value");
  }
  {
    auto r = run({"-x"});
    expect(r.parsed == cli::Parsed::Error && has_error(r, "unknown option '-x'"),
           "unknown short");
  }
  {
    auto r = run({"-rn", "3"});
    expect(r.parsed == cli::Parsed::Error, "combined short flags rejected");
  }
  {
    auto r = run({"--help", "--bogus"});
    expect(r.parsed == cli::Parsed::Help, "--help wins");
    auto v = run({"-V"});
    expect(v.parsed == cli::Parsed::Version, "-V");
  }

  // --- strict numbers
  {
    auto r = run({"--repeat", "abc", "--tie-pct", "-3"});
    r.args.get_int("repeat", 5, 1, 100);
    r.args.get_double("tie-pct", 5.0, 0.0, 100.0);
    expect(r.args.errors().size() == 2, "two typed errors collected");
    expect(has_error(r, "invalid value 'abc' for --repeat"), "int error text");
    expect(has_error(r, "for --tie-pct"), "double range error");
    auto r2 = run({"--repeat", "5x"});
    r2.args.get_int("repeat", 5, 1, 100);
    expect(!r2.args.errors().empty(), "trailing junk rejected");
    auto r3 = run({"--repeat", "1000"});
    r3.args.get_int("repeat", 5, 1, 100);
    expect(!r3.args.errors().empty(), "out of range rejected");
    auto r4 = run({"--tie-pct", "2.5"});
    expect(r4.args.get_double("tie-pct", 5.0, 0.0, 100.0) == 2.5, "double ok");
    auto r5 = run({});
    expect(r5.args.get_int("repeat", 5, 1, 100) == 5 && r5.args.errors().empty(), "default");
  }
  {
    std::vector<int> v;
    std::string err;
    expect(cli::parse_int_list("30,40,,40,50", 1, 100, v, err) && v.size() == 3, "list dedupe");
    expect(!cli::parse_int_list("30,x", 1, 100, v, err) && err.find("'x'") != std::string::npos,
           "list bad element named");
    expect(!cli::parse_int_list("30,101", 1, 100, v, err), "list range");
    expect(!cli::parse_int_list(",", 1, 100, v, err), "empty list rejected");
    auto r = run({"--repeat", "1,2", "--repeat", "2,3"});
    auto merged = r.args.get_int_list("repeat", {}, 1, 10);
    expect((merged == std::vector<int>{1, 2, 3}), "repeated list options merge");
  }

  // --- help text
  {
    std::ostringstream os;
    cli::print_help(os, make_spec());
    const std::string h = os.str();
    expect(h.rfind("Usage: prog [OPTION]... FILE...\n", 0) == 0, "usage line first");
    expect(h.find("\nOutput:\n") != std::string::npos && h.find("\nOptions:\n") != std::string::npos,
           "sections");
    expect(h.find("  -n, --repeat=N") != std::string::npos, "short+long+metavar");
    expect(h.find("      --json ") != std::string::npos, "long-only aligned");
    expect(h.find("-h, --help") != std::string::npos && h.find("-V, --version") != std::string::npos,
           "built-ins listed");
    expect(h.find("\nGeneral:\n") > h.find("\nOutput:\n"), "General section last of options");
    expect(h.find("\nExamples:\n  prog a.zip\n  prog --json dir/\n\n  prog -n 3 b.zip\n") !=
               std::string::npos,
           "free-text section verbatim, blank line kept");
    std::istringstream in(h);
    std::string line;
    std::size_t widest = 0;
    while (std::getline(in, line)) widest = std::max(widest, line.size());
    expect(widest <= 78, "help fits in 78 columns (widest " + std::to_string(widest) + ")");
    expect(h.find("--a-rather-long-option-name=SOMETHING\n") != std::string::npos,
           "overlong left column puts help on the next line");
  }

  // --- inputs
  {
    const fs::path root = fs::temp_directory_path() / ("gp_cli_test_" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root / "d" / "sub");
    for (const char* f : {"d/b.zip", "d/a.zip", "d/skip.txt", "d/sub/c.zip", "loose.dat"}) {
      std::ofstream(root / f) << "x";
    }
    const auto zip_only = [](const fs::path& p) { return p.extension() == ".zip"; };
    std::vector<std::string> errors;

    auto flat = cli::expand_inputs({(root / "d").string()}, false, zip_only, errors);
    expect(errors.empty() && flat.size() == 2 && flat[0].filename() == "a.zip" &&
               flat[1].filename() == "b.zip",
           "directory: filtered and sorted, non-recursive");

    auto rec = cli::expand_inputs({(root / "d").string()}, true, zip_only, errors);
    expect(rec.size() == 3, "recursive finds sub/c.zip");

    auto mixed = cli::expand_inputs({(root / "loose.dat").string(), (root / "d" / "a.zip").string(),
                                     (root / "d").string(), (root / ".." / root.filename() / "d" / "a.zip").string()},
                                    false, zip_only, errors);
    expect(mixed.size() == 3 && mixed[0].filename() == "loose.dat",
           "explicit file kept as given, duplicates dropped, order preserved");

    errors.clear();
    auto missing = cli::expand_inputs({(root / "nope.zip").string()}, false, zip_only, errors);
    expect(missing.empty() && errors.size() == 1 && errors[0].find("nope.zip") != std::string::npos,
           "missing path is an error");
    fs::remove_all(root);
  }

  if (g_fails) {
    std::cerr << g_fails << " failure(s)\n";
    return 1;
  }
  std::cout << "ok: gp_cli\n";
  return 0;
}
