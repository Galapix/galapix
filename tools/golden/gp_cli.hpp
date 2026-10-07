// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Command-line layer shared by the benchmark tools (gp-archive, gp-tile,
// microbench-decode, thumtoo-bench).
//
// A tool declares its options once in a Spec; the same table drives parsing,
// validation and --help, so the help text cannot drift from what is accepted.
//
//   const gp::cli::Spec spec{ .program = "thumtoo-gp-foo", ... };
//   gp::cli::Args args;
//   switch (gp::cli::parse(spec, argc, argv, args)) {
//     case Parsed::Help:    gp::cli::print_help(std::cout, spec); return 0;
//     case Parsed::Version: gp::cli::print_version(std::cout, spec.program); return 0;
//     case Parsed::Error:   return gp::cli::fail_usage(spec, args.errors());
//     case Parsed::Run:     break;
//   }
//   const int repeat = args.get_int("repeat", 5, 1, 1000);  // errors are collected
//   if (!args.errors().empty()) return gp::cli::fail_usage(spec, args.errors());
//
// Accepted syntax: --name VALUE, --name=VALUE, -x VALUE, -xVALUE, boolean
// flags, "--" to end options. A value-taking option consumes the next
// argument even if it starts with '-' (so "--max-scale -1" works). Options
// may repeat; the last value wins (Args::values() sees all). Unknown options
// are an error with a "did you mean" hint — never silently treated as files.
//
// Header-only, no thumtoo library dependency.

#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <unistd.h>

#if defined(__has_include)
#if __has_include("thumtoo/version.hpp")
#include "thumtoo/version.hpp"
#define GP_CLI_HAVE_VERSION 1
#endif
#endif
#ifndef GP_CLI_HAVE_VERSION
#define GP_CLI_HAVE_VERSION 0
#endif

namespace gp::cli {

namespace fs = std::filesystem;

// --- declarations ---------------------------------------------------------------

struct Option {
  std::string name;       // long name without dashes: "repeat"
  char short_name = 0;    // 'n' for -n, or 0
  std::string metavar;    // "N", "FILE"; empty = boolean flag
  std::string help;       // wrapped automatically
  std::string section = {};  // --help heading; empty = "Options"
};

struct Section {
  std::string title;
  std::string body;  // printed verbatim, indented two spaces
};

struct Spec {
  std::string program;
  std::string synopsis;  // after "Usage: PROGRAM "
  std::string summary;   // paragraphs separated by blank lines; wrapped
  std::vector<Option> options;
  std::vector<Section> sections = {};  // e.g. Examples, Exit status, Notes
};

enum class Parsed { Run, Help, Version, Error };

// --- strict value parsing -------------------------------------------------------

/// Whole-string base-10 integer; no trailing junk, no empty string.
inline std::optional<long long> parse_ll(std::string_view s) {
  if (s.empty()) return std::nullopt;
  const std::string tmp(s);
  char* end = nullptr;
  errno = 0;
  const long long v = std::strtoll(tmp.c_str(), &end, 10);
  if (errno != 0 || end == tmp.c_str() || *end != '\0') return std::nullopt;
  return v;
}

/// Whole-string finite double.
inline std::optional<double> parse_double(std::string_view s) {
  if (s.empty()) return std::nullopt;
  const std::string tmp(s);
  char* end = nullptr;
  errno = 0;
  const double v = std::strtod(tmp.c_str(), &end);
  if (errno != 0 || end == tmp.c_str() || *end != '\0' || v != v ||
      v > 1e300 || v < -1e300) {
    return std::nullopt;
  }
  return v;
}

/// Split on `sep`, dropping empty pieces ("a,,b," -> a, b).
inline std::vector<std::string> split(std::string_view s, char sep) {
  std::vector<std::string> out;
  std::size_t pos = 0;
  while (pos <= s.size()) {
    const std::size_t next = s.find(sep, pos);
    const std::string_view part =
        s.substr(pos, next == std::string_view::npos ? std::string_view::npos : next - pos);
    if (!part.empty()) out.emplace_back(part);
    if (next == std::string_view::npos) break;
    pos = next + 1;
  }
  return out;
}

/// "30,40,50" -> {30,40,50}, each within [lo, hi]; duplicates dropped.
/// On failure returns false and describes the first bad element in `err`.
inline bool parse_int_list(std::string_view s, int lo, int hi, std::vector<int>& out,
                           std::string& err) {
  out.clear();
  for (const std::string& part : split(s, ',')) {
    const auto v = parse_ll(part);
    if (!v || *v < lo || *v > hi) {
      err = "'" + part + "' is not an integer from " + std::to_string(lo) + " to " +
            std::to_string(hi);
      return false;
    }
    if (std::find(out.begin(), out.end(), static_cast<int>(*v)) == out.end()) {
      out.push_back(static_cast<int>(*v));
    }
  }
  if (out.empty()) {
    err = "expected a comma-separated list of integers";
    return false;
  }
  return true;
}

// --- parsed arguments -----------------------------------------------------------

class Args {
 public:
  bool has(std::string_view name) const {
    return std::any_of(opts_.begin(), opts_.end(),
                       [&](const auto& o) { return o.first == name; });
  }

  /// Last value given for `name`, or nullopt if absent. Flags yield "".
  std::optional<std::string> value(std::string_view name) const {
    for (auto it = opts_.rbegin(); it != opts_.rend(); ++it) {
      if (it->first == name) return it->second;
    }
    return std::nullopt;
  }

  /// Every value given for `name`, in order.
  std::vector<std::string> values(std::string_view name) const {
    std::vector<std::string> out;
    for (const auto& o : opts_) {
      if (o.first == name) out.push_back(o.second);
    }
    return out;
  }

  const std::vector<std::string>& positionals() const { return positionals_; }

  std::string get_string(std::string_view name, std::string def) const {
    auto v = value(name);
    return v ? *v : std::move(def);
  }

  /// Integer option in [lo, hi]; a bad value records an error and yields `def`.
  int get_int(std::string_view name, int def, int lo, int hi) {
    const auto v = value(name);
    if (!v) return def;
    const auto n = parse_ll(*v);
    if (!n || *n < lo || *n > hi) {
      add_error("invalid value '" + *v + "' for --" + std::string(name) +
                " (expected an integer from " + std::to_string(lo) + " to " +
                std::to_string(hi) + ")");
      return def;
    }
    return static_cast<int>(*n);
  }

  double get_double(std::string_view name, double def, double lo, double hi) {
    const auto v = value(name);
    if (!v) return def;
    const auto d = parse_double(*v);
    if (!d || *d < lo || *d > hi) {
      std::ostringstream range;
      range << lo << " to " << hi;
      add_error("invalid value '" + *v + "' for --" + std::string(name) +
                " (expected a number from " + range.str() + ")");
      return def;
    }
    return *d;
  }

  /// Comma-separated integer list option (several occurrences are merged).
  std::vector<int> get_int_list(std::string_view name, std::vector<int> def, int lo, int hi) {
    const auto vs = values(name);
    if (vs.empty()) return def;
    std::vector<int> out;
    for (const std::string& v : vs) {
      std::vector<int> part;
      std::string err;
      if (!parse_int_list(v, lo, hi, part, err)) {
        add_error("invalid value '" + v + "' for --" + std::string(name) + ": " + err);
        return def;
      }
      for (int x : part) {
        if (std::find(out.begin(), out.end(), x) == out.end()) out.push_back(x);
      }
    }
    return out;
  }

  const std::vector<std::string>& errors() const { return errors_; }
  void add_error(std::string e) { errors_.push_back(std::move(e)); }

  // Used by parse().
  void add_option(std::string name, std::string value) {
    opts_.emplace_back(std::move(name), std::move(value));
  }
  void add_positional(std::string p) { positionals_.push_back(std::move(p)); }

 private:
  std::vector<std::pair<std::string, std::string>> opts_;
  std::vector<std::string> positionals_;
  std::vector<std::string> errors_;
};

// --- parsing --------------------------------------------------------------------

namespace detail {

inline std::size_t edit_distance(std::string_view a, std::string_view b) {
  std::vector<std::size_t> prev(b.size() + 1), cur(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) prev[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    cur[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1,
                         prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1)});
    }
    std::swap(prev, cur);
  }
  return prev[b.size()];
}

/// Spec options plus the built-in --help / --version.
inline std::vector<Option> with_builtins(const Spec& spec) {
  std::vector<Option> all = spec.options;
  all.push_back({"help", 'h', "", "Show this help and exit.", "General"});
  all.push_back({"version", 'V', "", "Show the version and exit.", "General"});
  return all;
}

inline std::string suggestion(const std::vector<Option>& all, std::string_view typed) {
  std::string best;
  std::size_t best_d = std::string::npos;
  for (const Option& o : all) {
    const std::size_t d = edit_distance(typed, o.name);
    if (d < best_d) {
      best_d = d;
      best = o.name;
    }
  }
  const std::size_t limit = std::max<std::size_t>(2, typed.size() / 3);
  return best_d <= limit ? best : std::string();
}

}  // namespace detail

/// Parse argv against `spec`. Problems are recorded in args.errors() and
/// reported as Parsed::Error; the first --help / --version wins immediately.
inline Parsed parse(const Spec& spec, int argc, char** argv, Args& args) {
  const std::vector<Option> all = detail::with_builtins(spec);
  const auto find_long = [&](std::string_view n) -> const Option* {
    for (const Option& o : all) {
      if (o.name == n) return &o;
    }
    return nullptr;
  };
  const auto find_short = [&](char c) -> const Option* {
    for (const Option& o : all) {
      if (o.short_name == c) return &o;
    }
    return nullptr;
  };

  bool only_positionals = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (only_positionals || a.size() < 2 || a[0] != '-') {
      args.add_positional(a);  // includes a lone "-"
      continue;
    }
    if (a == "--") {
      only_positionals = true;
      continue;
    }

    const Option* opt = nullptr;
    std::optional<std::string> inline_value;
    std::string shown;  // the option as typed, for messages
    if (a[1] == '-') {
      const std::size_t eq = a.find('=');
      const std::string name = a.substr(2, eq == std::string::npos ? eq : eq - 2);
      if (eq != std::string::npos) inline_value = a.substr(eq + 1);
      shown = "--" + name;
      opt = find_long(name);
      if (!opt) {
        std::string msg = "unknown option '" + shown + "'";
        const std::string hint = detail::suggestion(all, name);
        if (!hint.empty()) msg += " (did you mean '--" + hint + "'?)";
        args.add_error(msg);
        continue;
      }
    } else {
      shown = a.substr(0, 2);
      opt = find_short(a[1]);
      if (!opt) {
        args.add_error("unknown option '" + shown + "'");
        continue;
      }
      if (a.size() > 2) inline_value = a.substr(2);  // -n5
    }

    if (opt->name == "help") return Parsed::Help;
    if (opt->name == "version") return Parsed::Version;

    if (opt->metavar.empty()) {
      if (a[1] == '-' && inline_value) {
        args.add_error("option '" + shown + "' does not take a value");
        continue;
      }
      if (a[1] != '-' && inline_value) {
        args.add_error("option '" + shown + "' does not take a value (combined short "
                       "options are not supported)");
        continue;
      }
      args.add_option(opt->name, "");
      continue;
    }
    if (inline_value) {
      args.add_option(opt->name, *inline_value);
    } else if (i + 1 < argc) {
      args.add_option(opt->name, argv[++i]);
    } else {
      args.add_error("option '" + shown + "' requires a value (" + opt->metavar + ")");
    }
  }
  return args.errors().empty() ? Parsed::Run : Parsed::Error;
}

// --- help / version / errors ----------------------------------------------------

namespace detail {

/// Greedy word wrap; words longer than `width` get their own line.
inline std::vector<std::string> wrap(std::string_view text, std::size_t width) {
  std::vector<std::string> lines;
  std::string line;
  std::istringstream in{std::string(text)};
  std::string word;
  while (in >> word) {
    if (!line.empty() && line.size() + 1 + word.size() > width) {
      lines.push_back(line);
      line.clear();
    }
    if (!line.empty()) line += ' ';
    line += word;
  }
  if (!line.empty()) lines.push_back(line);
  return lines;
}

inline std::string left_column(const Option& o) {
  std::string s = o.short_name ? std::string("  -") + o.short_name + ", " : "      ";
  s += "--" + o.name;
  if (!o.metavar.empty()) s += "=" + o.metavar;
  return s;
}

}  // namespace detail

inline constexpr std::size_t kHelpWidth = 78;

inline void print_help(std::ostream& os, const Spec& spec) {
  os << "Usage: " << spec.program << ' ' << spec.synopsis << "\n";

  // Summary paragraphs (separated by blank lines).
  std::string rest = spec.summary;
  while (!rest.empty()) {
    const std::size_t brk = rest.find("\n\n");
    const std::string para = rest.substr(0, brk);
    os << '\n';
    for (const std::string& l : detail::wrap(para, kHelpWidth)) os << l << '\n';
    if (brk == std::string::npos) break;
    rest.erase(0, brk + 2);
  }

  const std::vector<Option> all = detail::with_builtins(spec);
  constexpr std::size_t kMaxLeft = 30;
  std::size_t left_w = 0;
  for (const Option& o : all) {
    left_w = std::max(left_w, std::min(detail::left_column(o).size(), kMaxLeft));
  }
  const std::size_t help_col = left_w + 2;

  // Sections in order of first appearance; "General" (built-ins) last.
  std::vector<std::string> titles;
  for (const Option& o : all) {
    const std::string t = o.section.empty() ? "Options" : o.section;
    if (t != "General" && std::find(titles.begin(), titles.end(), t) == titles.end()) {
      titles.push_back(t);
    }
  }
  titles.push_back("General");

  for (const std::string& title : titles) {
    os << '\n' << title << ":\n";
    for (const Option& o : all) {
      if ((o.section.empty() ? "Options" : o.section) != title) continue;
      const std::string left = detail::left_column(o);
      const auto lines = detail::wrap(o.help, kHelpWidth - help_col);
      os << left;
      std::size_t col = left.size();
      if (left.size() > kMaxLeft || lines.empty()) {
        os << '\n';
        col = 0;
      }
      for (std::size_t i = 0; i < lines.size(); ++i) {
        os << std::string(help_col - col, ' ') << lines[i] << '\n';
        col = 0;
      }
    }
  }

  for (const Section& s : spec.sections) {
    os << '\n' << s.title << ":\n";
    std::istringstream in(s.body);
    std::string line;
    while (std::getline(in, line)) os << (line.empty() ? "" : "  ") << line << '\n';
  }
}

inline void print_version(std::ostream& os, const std::string& program) {
#if GP_CLI_HAVE_VERSION
  os << program << " (thumtoo " << thumtoo::version_string() << ")\n";
#else
  os << program << " (thumtoo, version unknown: built outside CMake)\n";
#endif
}

/// Print the collected errors GNU-style to stderr; returns the usage exit code 2.
inline int fail_usage(const Spec& spec, const std::vector<std::string>& errors) {
  for (const std::string& e : errors) std::cerr << spec.program << ": " << e << '\n';
  std::cerr << "Try '" << spec.program << " --help' for more information.\n";
  return 2;
}

// --- inputs ---------------------------------------------------------------------

/// Turn positional arguments into a list of input files.
///   * a regular file is taken as given (the tool decides what to do with it);
///   * a directory contributes its regular files for which `accept` is true,
///     sorted by name; with `recursive`, subdirectories too;
///   * anything else is an error (missing path, socket, ...).
/// Duplicates (same canonical file) are dropped, keeping the first. Problems
/// go to `errors`; a directory with no accepted files is a warning on stderr.
inline std::vector<fs::path> expand_inputs(const std::vector<std::string>& args,
                                           bool recursive,
                                           const std::function<bool(const fs::path&)>& accept,
                                           std::vector<std::string>& errors) {
  std::vector<fs::path> out;
  std::set<fs::path> seen;
  const auto add = [&](const fs::path& p) {
    std::error_code ec;
    fs::path key = fs::weakly_canonical(p, ec);
    if (ec) key = p;
    if (seen.insert(key).second) out.push_back(p);
  };

  for (const std::string& a : args) {
    std::error_code ec;
    const fs::path p(a);
    if (fs::is_directory(p, ec)) {
      std::vector<fs::path> found;
      const auto scan = [&](auto it) {
        for (const auto& entry : it) {
          std::error_code ec2;
          if (entry.is_regular_file(ec2) && accept(entry.path())) found.push_back(entry.path());
        }
      };
      if (recursive) {
        scan(fs::recursive_directory_iterator(p, fs::directory_options::skip_permission_denied, ec));
      } else {
        scan(fs::directory_iterator(p, fs::directory_options::skip_permission_denied, ec));
      }
      std::sort(found.begin(), found.end());
      if (found.empty()) {
        std::cerr << "warning: no supported files in " << a
                  << (recursive ? "" : " (use --recursive to search subdirectories)") << '\n';
      }
      for (const fs::path& f : found) add(f);
    } else if (fs::is_regular_file(p, ec)) {
      add(p);
    } else {
      errors.push_back("cannot read '" + a + "': no such file or directory");
    }
  }
  return out;
}

// --- progress -------------------------------------------------------------------

/// "[2/6] name" on stderr, rewritten in place — only when stderr is a terminal,
/// so logs and pipes stay clean. Call clear() before writing results.
class Progress {
 public:
  Progress() : tty_(::isatty(STDERR_FILENO) != 0) {}
  void update(std::size_t index, std::size_t total, std::string_view what) {
    if (!tty_) return;
    std::cerr << "\r\033[K[" << index << '/' << total << "] " << what << std::flush;
    shown_ = true;
  }
  void clear() {
    if (tty_ && shown_) std::cerr << "\r\033[K" << std::flush;
    shown_ = false;
  }

 private:
  bool tty_;
  bool shown_ = false;
};

}  // namespace gp::cli
