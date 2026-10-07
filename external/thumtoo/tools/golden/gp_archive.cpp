// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Golden-path archive timings (no thumtoo Client/Store): open the archive with
// the library directly, list it, extract everything, extract single members.
// Several backends and several archives can be measured in one run; with more
// than one backend the tool says which one wins. See --help.
//
// Every backend is verified before it is timed: extracted bytes must match
// the sizes its own TOC declares, and in comparison mode all backends must
// agree on member count and total bytes. A backend that fails verification
// is reported but never ranked.

#include <archive.h>
#include <archive_entry.h>

#include "gp_cli.hpp"
#include "gp_common.hpp"
#include "gp_json.hpp"
#include "gp_output.hpp"
#include "gp_verdict.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(THUMTOO_HAVE_UNARR) && THUMTOO_HAVE_UNARR
#include <unarr.h>
#define GP_ARCHIVE_HAVE_UNARR 1
#else
#define GP_ARCHIVE_HAVE_UNARR 0
#endif

namespace {

namespace fs = std::filesystem;

// --- format sniffing ------------------------------------------------------------

enum class Format { Zip, Rar4, Rar5, SevenZip, Tar, Other };

const char* format_name(Format f) {
  switch (f) {
    case Format::Zip: return "zip";
    case Format::Rar4: return "rar4";
    case Format::Rar5: return "rar5";
    case Format::SevenZip: return "7z";
    case Format::Tar: return "tar";
    case Format::Other: return "other";
  }
  return "other";
}

/// Container format by magic bytes (extension is not trusted: .cbz/.cbr lie).
/// Compressed tarballs (tar.gz, …) report Other: only libarchive has filters.
Format sniff_format(const fs::path& path) {
  std::FILE* f = std::fopen(path.string().c_str(), "rb");
  if (!f) return Format::Other;
  unsigned char buf[512] = {};
  const std::size_t n = std::fread(buf, 1, sizeof(buf), f);
  std::fclose(f);
  auto starts = [&](const char* magic, std::size_t len) {
    return n >= len && std::memcmp(buf, magic, len) == 0;
  };
  if (starts("PK\x03\x04", 4) || starts("PK\x05\x06", 4)) return Format::Zip;
  if (starts("Rar!\x1a\x07\x01\x00", 8)) return Format::Rar5;
  if (starts("Rar!\x1a\x07\x00", 7)) return Format::Rar4;
  if (starts("7z\xbc\xaf\x27\x1c", 6)) return Format::SevenZip;
  if (n >= 262 && std::memcmp(buf + 257, "ustar", 5) == 0) return Format::Tar;
  return Format::Other;
}

// --- backend interface ----------------------------------------------------------

struct Member {
  std::string path;
  std::optional<std::uint64_t> size;  // nullopt: TOC does not declare it
};

/// One archive library, used the obvious way: open, walk, read.
/// Every call re-opens the archive (that is the cost being measured).
class Backend {
 public:
  virtual ~Backend() = default;
  virtual const char* name() const = 0;
  /// Reason this backend cannot read `format`, or "" when it should.
  virtual std::string unsupported_reason(Format format) const = 0;
  /// True when support for `format` depends on how the library was built,
  /// so an open failure means "not compiled in" rather than "bad archive".
  virtual bool build_dependent(Format) const { return false; }
  /// Regular-file members in archive order; nullopt if the archive cannot be opened.
  virtual std::optional<std::vector<Member>> list() = 0;
  /// Bytes of one member; nullopt on open/read error or member not found.
  virtual std::optional<std::uint64_t> extract_one(const std::string& member) = 0;
  /// Bytes of all members in one sequential pass; nullopt on error.
  virtual std::optional<std::uint64_t> extract_all() = 0;
};

// --- libarchive -----------------------------------------------------------------

class LibarchiveBackend final : public Backend {
 public:
  explicit LibarchiveBackend(fs::path path) : path_(std::move(path)) {}
  const char* name() const override { return "libarchive"; }
  std::string unsupported_reason(Format) const override { return {}; }

  std::optional<std::vector<Member>> list() override {
    Handle h(path_);
    if (!h.ok()) return std::nullopt;
    std::vector<Member> out;
    struct archive_entry* entry = nullptr;
    int rc = ARCHIVE_OK;
    while ((rc = archive_read_next_header(h.a, &entry)) == ARCHIVE_OK) {
      const char* p = archive_entry_pathname(entry);
      if (p && archive_entry_filetype(entry) == AE_IFREG) {
        Member m;
        m.path = p;
        if (archive_entry_size_is_set(entry)) {
          m.size = static_cast<std::uint64_t>(archive_entry_size(entry));
        }
        out.push_back(std::move(m));
      }
      archive_read_data_skip(h.a);
    }
    if (rc != ARCHIVE_EOF) return std::nullopt;
    return out;
  }

  std::optional<std::uint64_t> extract_one(const std::string& member) override {
    Handle h(path_);
    if (!h.ok()) return std::nullopt;
    struct archive_entry* entry = nullptr;
    while (archive_read_next_header(h.a, &entry) == ARCHIVE_OK) {
      const char* p = archive_entry_pathname(entry);
      if (p && member == p && archive_entry_filetype(entry) == AE_IFREG) {
        return drain(h.a);
      }
      archive_read_data_skip(h.a);
    }
    return std::nullopt;
  }

  std::optional<std::uint64_t> extract_all() override {
    Handle h(path_);
    if (!h.ok()) return std::nullopt;
    struct archive_entry* entry = nullptr;
    std::uint64_t bytes = 0;
    int rc = ARCHIVE_OK;
    while ((rc = archive_read_next_header(h.a, &entry)) == ARCHIVE_OK) {
      if (archive_entry_filetype(entry) != AE_IFREG) continue;
      auto n = drain(h.a);
      if (!n) return std::nullopt;
      bytes += *n;
    }
    if (rc != ARCHIVE_EOF) return std::nullopt;
    return bytes;
  }

 private:
  struct Handle {
    struct archive* a = nullptr;
    bool opened = false;
    explicit Handle(const fs::path& path) : a(archive_read_new()) {
      archive_read_support_filter_all(a);
      archive_read_support_format_all(a);
      opened = archive_read_open_filename(a, path.string().c_str(), 10240) == ARCHIVE_OK;
    }
    ~Handle() { archive_read_free(a); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    bool ok() const { return opened; }
  };

  static std::optional<std::uint64_t> drain(struct archive* a) {
    char buf[16384];
    std::uint64_t bytes = 0;
    for (;;) {
      const la_ssize_t n = archive_read_data(a, buf, sizeof(buf));
      if (n == 0) return bytes;
      if (n < 0) return std::nullopt;
      bytes += static_cast<std::uint64_t>(n);
    }
  }

  fs::path path_;
};

// --- unarr (optional) -----------------------------------------------------------

#if GP_ARCHIVE_HAVE_UNARR

class UnarrBackend final : public Backend {
 public:
  enum class Access { Walk, Seek };

  UnarrBackend(fs::path path, Format format, Access access)
      : path_(std::move(path)), format_(format), access_(access) {}
  const char* name() const override {
    return access_ == Access::Seek ? "unarr-seek" : "unarr";
  }

  std::string unsupported_reason(Format format) const override {
    switch (format) {
      case Format::Zip:
      case Format::Rar4:
      case Format::Tar:
      case Format::SevenZip:  // see build_dependent()
        return {};
      case Format::Rar5: return "unarr does not support RAR5";
      case Format::Other: return "unarr has no reader for this container/filter";
    }
    return "unknown format";
  }

  bool build_dependent(Format format) const override {
    return format == Format::SevenZip;  // needs libunarr built with HAVE_7Z
  }

  std::optional<std::vector<Member>> list() override {
    Handle h(path_, format_);
    if (!h.ok()) return std::nullopt;
    std::vector<Member> out;
    offsets_.clear();
    while (ar_parse_entry(h.ar)) {
      const char* name = ar_entry_get_name(h.ar);
      if (!name || is_dir_name(name)) continue;
      out.push_back(Member{name, static_cast<std::uint64_t>(ar_entry_get_size(h.ar))});
      offsets_.emplace(name, ar_entry_get_offset(h.ar));
    }
    if (!ar_at_eof(h.ar)) return std::nullopt;
    return out;
  }

  std::optional<std::uint64_t> extract_one(const std::string& member) override {
    Handle h(path_, format_);
    if (!h.ok()) return std::nullopt;
    if (access_ == Access::Seek) {
      // Offsets come from the last list(); measure() always lists first.
      const auto it = offsets_.find(member);
      if (it == offsets_.end() || !ar_parse_entry_at(h.ar, it->second)) {
        return std::nullopt;
      }
      const char* name = ar_entry_get_name(h.ar);
      if (!name || member != name) return std::nullopt;  // landed elsewhere
      return uncompress_current(h.ar);
    }
    while (ar_parse_entry(h.ar)) {
      const char* name = ar_entry_get_name(h.ar);
      if (name && member == name) return uncompress_current(h.ar);
    }
    return std::nullopt;
  }

  std::optional<std::uint64_t> extract_all() override {
    Handle h(path_, format_);
    if (!h.ok()) return std::nullopt;
    std::uint64_t bytes = 0;
    while (ar_parse_entry(h.ar)) {
      const char* name = ar_entry_get_name(h.ar);
      if (!name || is_dir_name(name)) continue;
      auto n = uncompress_current(h.ar);
      if (!n) return std::nullopt;
      bytes += *n;
    }
    if (!ar_at_eof(h.ar)) return std::nullopt;
    return bytes;
  }

 private:
  struct Handle {
    ar_stream* stream = nullptr;
    ar_archive* ar = nullptr;
    Handle(const fs::path& path, Format format) {
      stream = ar_open_file(path.string().c_str());
      if (!stream) return;
      switch (format) {
        case Format::Zip: ar = ar_open_zip_archive(stream, false); break;
        case Format::Rar4: ar = ar_open_rar_archive(stream); break;
        case Format::Tar: ar = ar_open_tar_archive(stream); break;
        case Format::SevenZip: ar = ar_open_7z_archive(stream); break;
        case Format::Rar5:
        case Format::Other: break;
      }
    }
    ~Handle() {
      if (ar) ar_close_archive(ar);
      if (stream) ar_close(stream);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    bool ok() const { return ar != nullptr; }
  };

  static bool is_dir_name(const char* name) {
    const std::size_t len = std::strlen(name);
    return len > 0 && (name[len - 1] == '/' || name[len - 1] == '\\');
  }

  static std::optional<std::uint64_t> uncompress_current(ar_archive* ar) {
    const std::size_t declared = ar_entry_get_size(ar);
    if (declared == 0) return 0;
    std::vector<unsigned char> buf(declared);
    if (!ar_entry_uncompress(ar, buf.data(), declared)) return std::nullopt;
    return declared;
  }

  fs::path path_;
  Format format_;
  Access access_;
  std::unordered_map<std::string, off64_t> offsets_;  // member -> entry offset
};

#endif  // GP_ARCHIVE_HAVE_UNARR

std::vector<std::string> compiled_backends() {
  std::vector<std::string> out{"libarchive"};
#if GP_ARCHIVE_HAVE_UNARR
  out.push_back("unarr");
  out.push_back("unarr-seek");
#endif
  return out;
}

std::unique_ptr<Backend> make_backend(const std::string& name, const fs::path& path,
                                      Format format) {
  if (name == "libarchive") return std::make_unique<LibarchiveBackend>(path);
#if GP_ARCHIVE_HAVE_UNARR
  if (name == "unarr") {
    return std::make_unique<UnarrBackend>(path, format, UnarrBackend::Access::Walk);
  }
  if (name == "unarr-seek") {
    return std::make_unique<UnarrBackend>(path, format, UnarrBackend::Access::Seek);
  }
#else
  (void)format;
#endif
  return nullptr;
}

/// Mirrors thumtoo's dispatcher (archive_prefers_unarr): unarr for RAR4 only.
std::string auto_backend(Format format) {
#if GP_ARCHIVE_HAVE_UNARR
  if (format == Format::Rar4) return "unarr";
#else
  (void)format;
#endif
  return "libarchive";
}

// --- measurement ----------------------------------------------------------------

enum class Status { Ok, Unsupported, Failed };

const char* status_name(Status s) {
  switch (s) {
    case Status::Ok: return "ok";
    case Status::Unsupported: return "unsupported";
    case Status::Failed: return "failed";
  }
  return "failed";
}

constexpr int kScatterCount = 10;

struct Result {
  std::string backend;
  Status status = Status::Failed;
  std::string reason;
  std::size_t members = 0;
  std::uint64_t bytes = 0;  // verified extract-all total
  std::vector<Member> member_list;  // from verification; drives the timed ops
  gp::Timing toc, all, first, last, scattered;
};

const std::vector<gp::MetricSpec>& metric_specs() {
  static const std::vector<gp::MetricSpec> specs = {
      {"toc_ms", gp::Better::Lower, "table of contents", "TOC"},
      {"extract_all_ms", gp::Better::Lower, "extract all", "all"},
      {"extract_first_ms", gp::Better::Lower, "extract first", "first"},
      {"extract_last_ms", gp::Better::Lower, "extract last", "last"},
      {"extract_scattered10_ms", gp::Better::Lower, "extract 10 scattered", "scatter"},
  };
  return specs;
}

const gp::Timing* metric_timings(const Result& r, std::size_t i) {
  const gp::Timing* t[] = {&r.toc, &r.all, &r.first, &r.last, &r.scattered};
  return t[i];
}

gp::Candidate candidate(const Result& r) {
  gp::Candidate c{r.backend, {}, {}};
  for (std::size_t i = 0; i < metric_specs().size(); ++i) {
    const gp::Timing* t = metric_timings(r, i);
    c.values.push_back(t->median);
    c.spread.push_back({t->min, t->max});
  }
  return c;
}

std::vector<std::size_t> scattered_indices(std::size_t n) {
  std::vector<std::size_t> out;
  const std::size_t step = std::max<std::size_t>(1, n / kScatterCount);
  for (std::size_t i = 0; i < n; i += step) out.push_back(i);
  return out;
}

/// Check that `b` reads the archive correctly. No timing here: see time_all().
Result verify(Backend& b, Format format) {
  Result r;
  r.backend = b.name();
  if (auto why = b.unsupported_reason(format); !why.empty()) {
    r.status = Status::Unsupported;
    r.reason = why;
    return r;
  }
  auto listed = b.list();
  if (!listed) {
    if (b.build_dependent(format)) {
      r.status = Status::Unsupported;
      r.reason = std::string(b.name()) + " cannot open " + format_name(format) +
                 " (this libunarr build may lack 7z support)";
    } else {
      r.status = Status::Failed;
      r.reason = "could not read the " + std::string(format_name(format)) +
                 " archive (damaged or truncated?)";
    }
    return r;
  }
  const std::vector<Member>& members = *listed;
  if (members.empty()) {
    r.status = Status::Failed;
    r.reason = "no regular-file members";
    return r;
  }
  r.members = members.size();

  // Verification: one full pass plus the single-member probes we will time.
  std::uint64_t declared = 0;
  bool all_declared = true;
  for (const auto& m : members) {
    if (m.size) declared += *m.size;
    else all_declared = false;
  }
  const auto all_bytes = b.extract_all();
  if (!all_bytes) {
    r.status = Status::Failed;
    r.reason = "extract-all failed";
    return r;
  }
  if (all_declared && *all_bytes != declared) {
    std::ostringstream os;
    os << "extract-all produced " << *all_bytes << " bytes, TOC declares " << declared;
    r.status = Status::Failed;
    r.reason = os.str();
    return r;
  }
  // Every member a timed op extracts: first, last and the scattered set.
  const auto scatter = scattered_indices(members.size());
  std::vector<std::size_t> probe = scatter;
  probe.push_back(0);
  probe.push_back(members.size() - 1);
  std::sort(probe.begin(), probe.end());
  probe.erase(std::unique(probe.begin(), probe.end()), probe.end());
  for (std::size_t i : probe) {
    const Member& m = members[i];
    const auto got = b.extract_one(m.path);
    if (!got || (m.size && *got != *m.size)) {
      r.status = Status::Failed;
      r.reason = "extract of member '" + m.path + "' failed or was short";
      return r;
    }
  }
  r.bytes = *all_bytes;
  r.member_list = members;
  r.status = Status::Ok;
  return r;
}

/// Time every verified backend, one op at a time, interleaving backends
/// round-robin (gp::time_interleaved) so run order does not pick the winner.
/// Without this, whichever backend ran second measured ~10-15% faster on
/// identical code paths.
void time_all(std::vector<std::unique_ptr<Backend>>& backends,
              std::vector<Result>& results, int repeats) {
  std::vector<std::size_t> ok;
  for (std::size_t i = 0; i < results.size(); ++i) {
    if (results[i].status == Status::Ok) ok.push_back(i);
  }
  if (ok.empty()) return;
  const int heavy = std::max(1, repeats / 2);

  using Op = std::function<void(Backend&, const std::vector<Member>&)>;
  auto run = [&](int reps, const Op& op, gp::Timing Result::*field) {
    std::vector<std::function<void()>> fns;
    for (std::size_t i : ok) {
      Backend& b = *backends[i];
      const auto& members = results[i].member_list;
      fns.push_back([&b, &members, &op] { op(b, members); });
    }
    const auto timings = gp::time_interleaved(reps, fns);
    for (std::size_t k = 0; k < ok.size(); ++k) results[ok[k]].*field = timings[k];
  };

  run(repeats, [](Backend& b, const auto&) { (void)b.list(); }, &Result::toc);
  run(heavy, [](Backend& b, const auto&) { (void)b.extract_all(); }, &Result::all);
  run(repeats, [](Backend& b, const auto& m) { (void)b.extract_one(m.front().path); },
      &Result::first);
  run(repeats, [](Backend& b, const auto& m) { (void)b.extract_one(m.back().path); },
      &Result::last);
  run(heavy,
      [](Backend& b, const auto& m) {
        for (std::size_t i : scattered_indices(m.size())) (void)b.extract_one(m[i].path);
      },
      &Result::scattered);
}

/// In comparison mode every verified backend must see the same archive.
/// Returns "" when consistent, else a description of the disagreement.
std::string cross_check(const std::vector<Result>& results) {
  const Result* ref = nullptr;
  for (const auto& r : results) {
    if (r.status != Status::Ok) continue;
    if (!ref) {
      ref = &r;
      continue;
    }
    if (r.members != ref->members || r.bytes != ref->bytes) {
      std::ostringstream os;
      os << ref->backend << " sees " << ref->members << " members / " << ref->bytes
         << " bytes, " << r.backend << " sees " << r.members << " members / "
         << r.bytes << " bytes";
      return os.str();
    }
  }
  return {};
}

// --- backend selection ---------------------------------------------------------

struct BackendInfo {
  const char* name;
  const char* help;
  bool built;
};

const std::vector<BackendInfo>& backend_infos() {
  static const std::vector<BackendInfo> infos = {
      {"libarchive",
       "libarchive. Reads every format (zip, rar, rar5, 7z, tar, compressed tar, ...).",
       true},
      {"unarr",
       "libunarr. Reads zip, tar, rar4 (also solid) and, if libunarr was built with the 7z "
       "SDK, 7z; not rar5. Finds a single member by walking the entries from the start.",
       GP_ARCHIVE_HAVE_UNARR != 0},
      {"unarr-seek",
       "unarr, but single-member extracts jump to the entry offset recorded during the TOC "
       "read (the route thumtoo can take because it caches the TOC). Its first/last/"
       "scattered timings therefore exclude the TOC.",
       GP_ARCHIVE_HAVE_UNARR != 0},
  };
  return infos;
}

/// Which backends to run on each archive.
struct BackendSel {
  bool automatic = false;  // thumtoo's dispatcher choice, decided per archive
  bool compare = false;    // more than one backend: judge which wins
  std::vector<std::string> names;

  std::vector<std::string> for_format(Format f) const {
    return automatic ? std::vector<std::string>{auto_backend(f)} : names;
  }
};

/// "auto", "all", or "a,b,..."; the problem is described in `err` on failure.
std::optional<BackendSel> parse_backend_sel(const std::string& flag, std::string& err) {
  BackendSel sel;
  if (flag == "auto") {
    sel.automatic = true;
    return sel;
  }
  if (flag == "all") {
    sel.names = compiled_backends();
    sel.compare = true;
    return sel;
  }
  const auto built = compiled_backends();
  for (const std::string& n : gp::cli::split(flag, ',')) {
    if (std::find(sel.names.begin(), sel.names.end(), n) != sel.names.end()) continue;
    if (std::find(built.begin(), built.end(), n) == built.end()) {
      const auto known = std::find_if(backend_infos().begin(), backend_infos().end(),
                                      [&](const BackendInfo& b) { return n == b.name; });
      if (known != backend_infos().end()) {
        err = "backend '" + n + "' is not available in this build (built without libunarr)";
      } else {
        err = "unknown backend '" + n + "' (use auto, all, or a list of:";
        for (const std::string& b : built) err += " " + b;
        err += ")";
      }
      return std::nullopt;
    }
    sel.names.push_back(n);
  }
  if (sel.names.empty()) {
    err = "expected auto, all, or a comma-separated list of backends";
    return std::nullopt;
  }
  sel.compare = sel.names.size() > 1;
  return sel;
}

// --- measuring one archive ------------------------------------------------------

/// Verdict for one archive's comparison, or the reason none can be given.
struct Judged {
  std::string mismatch;  // backends disagree about the archive (cross_check)
  gp::Verdict verdict;
  /// Comparable: consistent and at least one verified backend.
  bool ok() const { return mismatch.empty() && !verdict.empty(); }
};

Judged judge_results(const std::vector<Result>& results, double tie_pct) {
  Judged j;
  j.mismatch = cross_check(results);
  std::vector<gp::Candidate> candidates;
  if (j.mismatch.empty()) {
    for (const auto& r : results) {
      if (r.status == Status::Ok) candidates.push_back(candidate(r));
    }
  }
  j.verdict = gp::judge(metric_specs(), candidates, tie_pct);
  return j;
}

struct ArchiveRun {
  fs::path path;
  std::string name;  // display name
  Format format = Format::Other;
  std::vector<Result> results;
  bool compare = false;
  Judged judged;  // compare mode only

  /// Measured (single backend) / a verdict could be reached (comparison).
  bool ok() const {
    return compare ? judged.ok()
                   : (!results.empty() && results.front().status == Status::Ok);
  }
  const Result* any_ok() const {
    for (const Result& r : results) {
      if (r.status == Status::Ok) return &r;
    }
    return nullptr;
  }
};

ArchiveRun measure_archive(const fs::path& path, const std::string& name,
                           const BackendSel& sel, int repeats, double tie_pct) {
  gp::StderrSilencer quiet;  // libunarr logs on every open
  ArchiveRun run;
  run.path = path;
  run.name = name;
  run.format = sniff_format(path);
  run.compare = sel.compare;
  std::vector<std::unique_ptr<Backend>> backends;
  for (const std::string& n : sel.for_format(run.format)) {
    backends.push_back(make_backend(n, path, run.format));
    run.results.push_back(verify(*backends.back(), run.format));
  }
  time_all(backends, run.results, repeats);
  if (run.compare) run.judged = judge_results(run.results, tie_pct);
  return run;
}

// --- JSON documents -------------------------------------------------------------

void write_metrics_json(gp::JsonWriter& w, const Result& r) {
  w.begin_object();
  w.field("toc_ms", r.toc.median);
  w.field("extract_all_ms", r.all.median);
  w.field("extract_first_ms", r.first.median);
  w.field("extract_last_ms", r.last.median);
  w.field("extract_scattered10_ms", r.scattered.median);
  w.end_object();
}

void write_header_json(gp::JsonWriter& w, const ArchiveRun& run) {
  w.field("tool", "thumtoo-gp-archive");
  w.field("archive", run.path.string());
  w.field("format", format_name(run.format));
  w.field("unarr_built", GP_ARCHIVE_HAVE_UNARR != 0);
}

/// One backend, one archive. Keys are kept stable for checked-in baselines;
/// "status" (and "reason" instead of members/metrics on failure) is additive.
void write_single_doc(gp::JsonWriter& w, const ArchiveRun& run) {
  const Result& r = run.results.front();
  w.begin_object();
  w.field("schema", 1);
  write_header_json(w, run);
  w.field("backend", r.backend);
  w.field("status", status_name(r.status));
  if (r.status == Status::Ok) {
    w.field("members", r.members);
    w.key("metrics");
    write_metrics_json(w, r);
  } else {
    w.field("reason", r.reason);
  }
  w.end_object();
}

void write_compare_doc(gp::JsonWriter& w, const ArchiveRun& run, int repeats) {
  w.begin_object();
  w.field("schema", 1);
  w.field("kind", "compare");
  write_header_json(w, run);
  w.field("repeats", repeats);
  w.key("variants").begin_array();
  for (const Result& r : run.results) {
    w.begin_object(gp::JsonWriter::Compact);
    w.field("backend", r.backend);
    w.field("status", status_name(r.status));
    if (r.status == Status::Ok) {
      w.field("members", r.members);
      w.field("bytes", r.bytes);
      w.key("metrics");
      write_metrics_json(w, r);
    } else {
      w.field("reason", r.reason);
    }
    w.end_object();
  }
  w.end_array();
  w.field("consistency_error", run.judged.mismatch);
  w.key("verdict");
  gp::write_verdict_json(w, run.judged.verdict);
  w.end_object();
}

// --- CSV rows -------------------------------------------------------------------

const std::vector<std::string>& csv_columns() {
  static const std::vector<std::string> cols = {
      "archive",         "format",           "backend",          "status",
      "reason",          "members",          "bytes",            "toc_ms",
      "extract_all_ms",  "extract_first_ms", "extract_last_ms",  "extract_scattered10_ms",
      "overall_ratio"};
  return cols;
}

std::vector<std::string> csv_row(const ArchiveRun& run, const Result& r) {
  const bool ok = r.status == Status::Ok;
  std::string reason = r.reason;
  if (ok && run.compare && !run.judged.mismatch.empty()) {
    reason = "backends disagree: " + run.judged.mismatch;
  }
  std::string ratio;
  // A ratio means "cost relative to the best of several"; with a single
  // measurable backend there is nothing to be relative to.
  if (run.compare && run.judged.verdict.overall.size() > 1) {
    if (const auto v = run.judged.verdict.overall_ratio(r.backend)) ratio = gp::csv_num(*v);
  }
  const auto num = [&](double v) { return ok ? gp::csv_num(v) : std::string(); };
  return {run.path.string(),
          format_name(run.format),
          r.backend,
          status_name(r.status),
          reason,
          ok ? std::to_string(r.members) : "",
          ok ? std::to_string(r.bytes) : "",
          num(r.toc.median),
          num(r.all.median),
          num(r.first.median),
          num(r.last.median),
          num(r.scattered.median),
          ratio};
}

// --- text report ----------------------------------------------------------------

/// "name  (zip, 160 members, 49.0 MiB)"
void print_archive_heading(const ArchiveRun& run) {
  std::cout << run.name << "  (" << format_name(run.format);
  if (const Result* r = run.any_ok()) {
    std::cout << ", " << r->members << " members, " << gp::fmt_bytes(r->bytes);
  }
  std::cout << ")\n";
}

void print_compare_block(const ArchiveRun& run) {
  using gp::Align;
  print_archive_heading(run);
  gp::TextTable table({{"backend", Align::Left},
                       {"TOC ms", Align::Right},
                       {"all ms", Align::Right},
                       {"first ms", Align::Right},
                       {"last ms", Align::Right},
                       {"10 scattered ms", Align::Right}});
  for (const Result& r : run.results) {
    if (r.status != Status::Ok) continue;
    table.add_row({r.backend, gp::fmt_ms(r.toc.median), gp::fmt_ms(r.all.median),
                   gp::fmt_ms(r.first.median), gp::fmt_ms(r.last.median),
                   gp::fmt_ms(r.scattered.median)});
  }
  if (!table.empty()) table.print(std::cout, "  ");
  for (const Result& r : run.results) {
    if (r.status != Status::Ok) {
      std::cout << "  " << r.backend << ": " << status_name(r.status) << " — " << r.reason << "\n";
    }
  }
  if (!run.judged.mismatch.empty()) {
    std::cout << "  verdict refused: backends disagree about the archive: "
              << run.judged.mismatch << "\n";
  } else if (run.any_ok()) {
    std::ostringstream verdict;
    gp::print_verdict(verdict, run.judged.verdict);
    std::cout << "\n" << gp::indent_lines(verdict.str(), "  ");
  }
  std::cout << "\n";
}

/// Collects one row per archive for the single-backend report.
class SingleTable {
 public:
  SingleTable()
      : table_({{"archive", gp::Align::Left},
                {"format", gp::Align::Left},
                {"backend", gp::Align::Left},
                {"members", gp::Align::Right},
                {"size", gp::Align::Right},
                {"TOC ms", gp::Align::Right},
                {"all ms", gp::Align::Right},
                {"first ms", gp::Align::Right},
                {"last ms", gp::Align::Right},
                {"10 scattered ms", gp::Align::Right}}) {}

  void add(const ArchiveRun& run) {
    const Result& r = run.results.front();
    if (r.status != Status::Ok) {
      failures_.push_back(run.name + ": " + r.backend + ": " + status_name(r.status) +
                          " — " + r.reason);
      return;
    }
    table_.add_row({run.name, format_name(run.format), r.backend, std::to_string(r.members),
                    gp::fmt_bytes(r.bytes), gp::fmt_ms(r.toc.median), gp::fmt_ms(r.all.median),
                    gp::fmt_ms(r.first.median), gp::fmt_ms(r.last.median),
                    gp::fmt_ms(r.scattered.median)});
  }

  void print() const {
    if (!table_.empty()) table_.print(std::cout, "");
    for (const std::string& f : failures_) std::cout << f << "\n";
  }

 private:
  gp::TextTable table_;
  std::vector<std::string> failures_;
};

// --- reporter -------------------------------------------------------------------

/// Receives each archive as it finishes and renders it in the chosen format.
/// Text (comparison) and CSV/JSON output stream; the single-backend text table
/// needs all rows to align and is printed by finish().
class Reporter {
 public:
  Reporter(gp::OutputMode mode, bool compare, bool csv_header, int repeats, double tie_pct,
           std::size_t total)
      : mode_(mode),
        compare_(compare),
        repeats_(repeats),
        total_(total),
        agg_(metric_specs(), tie_pct) {
    if (mode_ == gp::OutputMode::Csv) {
      csv_.emplace(std::cout, csv_columns(), csv_header);
    } else if (mode_ == gp::OutputMode::Json && total_ > 1) {
      json_.emplace(std::cout);
      json_->begin_object();
      json_->field("schema", 1);
      json_->field("kind", "batch");
      json_->field("tool", "thumtoo-gp-archive");
      json_->field("repeats", repeats_);
      json_->key("results").begin_array();
    }
  }

  void add(const ArchiveRun& run) {
    if (!run.ok()) ++failures_;
    if (compare_) agg_.add(run.judged.verdict);
    switch (mode_) {
      case gp::OutputMode::Text:
        if (compare_) print_compare_block(run);
        else single_.add(run);
        break;
      case gp::OutputMode::Csv:
        for (const Result& r : run.results) csv_->row(csv_row(run, r));
        break;
      case gp::OutputMode::Json:
        if (json_) {
          emit_json(*json_, run);
        } else {
          gp::JsonWriter w(std::cout);  // a single archive: the bare document
          emit_json(w, run);
        }
        break;
    }
  }

  /// Trailing output; returns the process exit status (1 if anything failed).
  int finish() {
    if (mode_ == gp::OutputMode::Text) {
      if (!compare_) {
        single_.print();
      } else if (total_ > 1) {
        gp::print_aggregate(std::cout, agg_.result(), "archives");
      }
    } else if (mode_ == gp::OutputMode::Json && json_) {
      json_->end_array();
      if (compare_) {
        json_->key("aggregate");
        gp::write_aggregate_json(*json_, agg_.result());
      }
      json_->end_object();
    }
    if (failures_ > 0) {
      std::cerr << "thumtoo-gp-archive: " << failures_ << " of " << total_
                << (total_ == 1 ? " archive" : " archives")
                << (compare_ ? " could not be compared" : " could not be measured") << "\n";
      return 1;
    }
    return 0;
  }

 private:
  void emit_json(gp::JsonWriter& w, const ArchiveRun& run) const {
    if (compare_) write_compare_doc(w, run, repeats_);
    else write_single_doc(w, run);
  }

  gp::OutputMode mode_;
  bool compare_;
  int repeats_;
  std::size_t total_;
  gp::Aggregator agg_;
  SingleTable single_;
  std::optional<gp::CsvWriter> csv_;
  std::optional<gp::JsonWriter> json_;
  std::size_t failures_ = 0;
};

// --- command line ---------------------------------------------------------------

gp::cli::Spec make_spec() {
  gp::cli::Spec s;
  s.program = "thumtoo-gp-archive";
  s.synopsis = "[OPTION]... ARCHIVE|DIR...";
  s.summary =
      "Time archive readers the way an application uses them: list the table of "
      "contents, extract everything, and extract single members (the first, the last "
      "and ten spread across the archive). With more than one backend the tool also "
      "says which one wins, per metric and overall.\n\n"
      "Archives are opened with the library directly, without thumtoo's cache or "
      "database, so the numbers show what the library alone costs. Timings are medians "
      "of --repeat runs after one warm-up on a warm page cache: they measure CPU and "
      "library overhead, not the disk. Backends are timed interleaved, so run order "
      "does not favor one. Before anything is timed, each backend must extract exactly "
      "the bytes its own table of contents declares; a backend that fails this is "
      "reported and never ranked.";
  s.options = {
      {"backend", 'b', "LIST",
       "Backends to run: auto (thumtoo's choice per archive, the default), all (every "
       "backend in this build, then judge which wins), or a comma-separated list "
       "such as libarchive,unarr. A list with more than one entry is a comparison."},
      {"repeat", 'n', "N", "Timed runs per measurement, 1-1000 (default 5)."},
      {"tie-pct", 0, "PCT",
       "Comparison only: results within PCT percent of the best, or within run-to-run "
       "noise, count as a tie, 0-100 (default 5)."},
      {"recursive", 'r', "",
       "Search subdirectories of any DIR as well. Directories are searched for "
       "archives by content (zip, rar, 7z, tar), sorted by name."},
  };
  for (const auto& o : gp::output_options()) s.options.push_back(o);

  std::string backends;
  for (const BackendInfo& b : backend_infos()) {
    const std::string text = std::string(b.help) + (b.built ? "" : " NOT AVAILABLE IN THIS BUILD.");
    const auto lines = gp::cli::detail::wrap(text, 58);
    for (std::size_t i = 0; i < lines.size(); ++i) {
      char head[32];
      std::snprintf(head, sizeof(head), "%-12s", i == 0 ? b.name : "");
      backends += std::string(head) + " " + lines[i] + "\n";
    }
  }
  s.sections = {
      {"Backends", backends},
      {"Output formats",
       "Default: aligned tables and a verdict in words, for reading. With several\n"
       "archives and a comparison, a summary over all archives follows.\n"
       "--csv:   one row per archive and backend; raw milliseconds and bytes; empty\n"
       "         cell = not measured. overall_ratio is the cost relative to the best\n"
       "         backend on that archive (1 = best).\n"
       "--json:  one document for one archive; with several archives a batch document\n"
       "         {kind: \"batch\", results: [...], aggregate: {...}}.\n"
       "Warnings and errors go to stderr, so stdout is clean for pipes."},
      {"Exit status",
       "0  every archive was measured (comparison: and a verdict could be reached)\n"
       "1  an archive could not be read, or the backends disagreed about its contents\n"
       "2  usage error"},
      {"Examples",
       "thumtoo-gp-archive book.cbz\n"
       "thumtoo-gp-archive --backend all corpus/archives/\n"
       "thumtoo-gp-archive -b libarchive,unarr -n 10 *.cbz\n"
       "thumtoo-gp-archive --csv --backend all -r corpus/ > archives.csv\n"
       "thumtoo-gp-archive --json --backend all comic.cbr"},
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
  const double tie_pct = args.get_double("tie-pct", 5.0, 0.0, 100.0);
  const gp::OutputMode mode = gp::resolve_output_mode(args);
  std::string backend_err;
  const auto sel = parse_backend_sel(args.get_string("backend", "auto"), backend_err);
  if (!sel) args.add_error("--backend: " + backend_err);

  if (args.positionals().empty()) args.add_error("missing ARCHIVE argument");
  std::vector<std::string> input_errors;
  const std::vector<fs::path> inputs = cli::expand_inputs(
      args.positionals(), args.has("recursive"),
      [](const fs::path& p) { return sniff_format(p) != Format::Other; }, input_errors);
  for (const std::string& e : input_errors) args.add_error(e);
  if (args.errors().empty() && inputs.empty()) args.add_error("no archives to measure");
  if (!args.errors().empty()) return cli::fail_usage(spec, args.errors());

  const std::vector<std::string> names = gp::display_names(inputs);
  Reporter reporter(mode, sel->compare, !args.has("no-header"), repeats, tie_pct, inputs.size());
  cli::Progress progress;
  for (std::size_t i = 0; i < inputs.size(); ++i) {
    progress.update(i + 1, inputs.size(), names[i]);
    const ArchiveRun run = measure_archive(inputs[i], names[i], *sel, repeats, tie_pct);
    progress.clear();
    reporter.add(run);
  }
  return reporter.finish();
}
