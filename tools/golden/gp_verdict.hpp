// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// "Who wins?" judgement for golden-tool comparisons (archive backends,
// tile codecs, …). Pure data in, verdict out — no I/O dependencies beyond
// <ostream>, so it is unit-testable (tests/test_gp_verdict.cpp).
//
// Rules:
//   * Per metric, every candidate gets a cost ratio >= 1 relative to the best
//     candidate (1.0 = best). Lower-is-better: v / best. Higher-is-better:
//     best / v.
//   * Candidates within `tie_pct` percent of the best are tied with it.
//   * Noise: when candidates carry per-metric spreads (min/max of the timed
//     runs), a candidate is also tied with the best if even its best run is
//     no better than the best candidate's value (median): the measured gap
//     is within run-to-run noise. Deterministic metrics (bytes) use a
//     zero-width spread and are decided by the band alone.
//   * Overall score = geometric mean of a candidate's per-metric ratios
//     (all metrics weighted equally; scale-free so ms and bytes mix).
//     The same tie band applies to the overall score.
//
// Callers are responsible for only passing candidates whose results are
// valid (verified output, codec supported, …). A failed run must never be
// a candidate: it would "win" with zero time / zero bytes.

#pragma once

#include "gp_common.hpp"
#include "gp_json.hpp"
#include "gp_output.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

namespace gp {

enum class Better { Lower, Higher };

struct MetricSpec {
  std::string name;  // stable key: JSON / CSV
  Better better = Better::Lower;
  std::string label = {};        // human-readable text report; empty = name
  std::string short_label = {};  // narrow table headers; empty = label
};

struct Spread {
  double min = 0;
  double max = 0;
};

struct Candidate {
  std::string name;
  std::vector<double> values;  // aligned with the MetricSpec list (medians)
  std::vector<Spread> spread = {};  // empty, or aligned with values (min/max of runs)
};

struct Ranked {
  std::string name;
  double ratio = 1.0;  // >= 1; 1.0 = best
};

struct MetricVerdict {
  std::string metric;
  std::string label;  // human-readable name (never empty)
  Better better = Better::Lower;
  std::vector<Ranked> ranking;  // best first
  // Names tied with the best (incl. best): within the band, or within noise.
  std::vector<std::string> tied;

  bool decided() const { return ranking.size() > 1 && tied.size() == 1; }
  const std::string& winner() const { return ranking.front().name; }
};

struct Verdict {
  double tie_pct = 5.0;
  std::vector<MetricVerdict> metrics;
  std::vector<Ranked> overall;  // geomean score ranking, best first
  std::vector<std::string> overall_tied;

  bool empty() const { return overall.empty(); }
  bool overall_decided() const {
    return overall.size() > 1 && overall_tied.size() == 1;
  }
  /// Overall cost ratio (1.0 = best) of a candidate, if it was ranked.
  std::optional<double> overall_ratio(const std::string& name) const {
    for (const auto& r : overall) {
      if (r.name == name) return r.ratio;
    }
    return std::nullopt;
  }
};

namespace detail {

// Guard against 0 ms / 0 bytes producing infinities. Values this small are
// far below timer resolution and never meaningful as a "win".
inline constexpr double kFloor = 1e-9;

inline double cost_ratio(double v, double best, Better better) {
  v = std::max(v, kFloor);
  best = std::max(best, kFloor);
  return better == Better::Lower ? v / best : best / v;
}

inline std::vector<std::string> tie_band(const std::vector<Ranked>& ranking,
                                         double tie_pct) {
  std::vector<std::string> out;
  const double limit = 1.0 + tie_pct / 100.0;
  for (const auto& r : ranking) {
    if (r.ratio <= limit) out.push_back(r.name);
  }
  return out;
}

/// Is `c`'s best run no better than `best`'s typical (median) value?
inline bool within_noise(const Candidate& c, const Candidate& best, std::size_t m,
                         Better better) {
  if (c.spread.size() != c.values.size()) return false;
  const double typical = best.values[m];
  return better == Better::Lower ? c.spread[m].min <= typical
                                 : c.spread[m].max >= typical;
}

inline void sort_ranking(std::vector<Ranked>& ranking) {
  std::stable_sort(ranking.begin(), ranking.end(),
                   [](const Ranked& a, const Ranked& b) { return a.ratio < b.ratio; });
}

}  // namespace detail

/// Judge `candidates` on `metrics`. Candidates with a non-finite value or a
/// value count that does not match `metrics` are ignored.
inline Verdict judge(const std::vector<MetricSpec>& metrics,
                     const std::vector<Candidate>& candidates,
                     double tie_pct = 5.0) {
  Verdict v;
  v.tie_pct = tie_pct;

  std::vector<const Candidate*> valid;
  for (const auto& c : candidates) {
    if (c.values.size() != metrics.size()) continue;
    if (!c.spread.empty() && c.spread.size() != c.values.size()) continue;
    const bool finite = std::all_of(c.values.begin(), c.values.end(),
                                    [](double x) { return std::isfinite(x); });
    if (finite) valid.push_back(&c);
  }
  if (valid.empty() || metrics.empty()) return v;

  std::vector<double> log_sum(valid.size(), 0.0);
  for (std::size_t m = 0; m < metrics.size(); ++m) {
    const Better better = metrics[m].better;
    double best = valid.front()->values[m];
    for (const auto* c : valid) {
      const double x = c->values[m];
      best = better == Better::Lower ? std::min(best, x) : std::max(best, x);
    }
    MetricVerdict mv;
    mv.metric = metrics[m].name;
    mv.label = metrics[m].label.empty() ? metrics[m].name : metrics[m].label;
    mv.better = better;
    for (std::size_t i = 0; i < valid.size(); ++i) {
      const double r = detail::cost_ratio(valid[i]->values[m], best, better);
      mv.ranking.push_back({valid[i]->name, r});
      log_sum[i] += std::log(r);
    }
    detail::sort_ranking(mv.ranking);
    mv.tied = detail::tie_band(mv.ranking, tie_pct);
    const auto by_name = [&](const std::string& n) {
      return *std::find_if(valid.begin(), valid.end(),
                           [&](const Candidate* c) { return c->name == n; });
    };
    const Candidate* best_c = by_name(mv.ranking.front().name);
    for (const auto& r : mv.ranking) {
      if (std::find(mv.tied.begin(), mv.tied.end(), r.name) != mv.tied.end()) continue;
      if (detail::within_noise(*by_name(r.name), *best_c, m, better)) {
        mv.tied.push_back(r.name);
      }
    }
    v.metrics.push_back(std::move(mv));
  }

  for (std::size_t i = 0; i < valid.size(); ++i) {
    const double geo = std::exp(log_sum[i] / static_cast<double>(metrics.size()));
    v.overall.push_back({valid[i]->name, geo});
  }
  detail::sort_ranking(v.overall);
  // Normalize so the best overall score is exactly 1.0 (geomeans of
  // per-metric ratios need not include a candidate that is best everywhere).
  const double best_score = v.overall.front().ratio;
  for (auto& r : v.overall) r.ratio /= best_score;
  v.overall_tied = detail::tie_band(v.overall, tie_pct);
  return v;
}

// --- output -------------------------------------------------------------------

inline std::string join_names(const std::vector<std::string>& names) {
  std::string out;
  for (const auto& n : names) {
    if (!out.empty()) out += " = ";
    out += n;
  }
  return out;
}

/// Human-readable verdict block.
inline void print_verdict(std::ostream& os, const Verdict& v) {
  if (v.empty()) {
    os << "verdict: no valid candidates\n";
    return;
  }
  if (v.overall.size() == 1) {
    os << "verdict: only one valid candidate (" << v.overall.front().name
       << ") — nothing to compare\n";
    return;
  }
  std::size_t label_w = 0;
  std::size_t name_w = 0;
  for (const auto& m : v.metrics) {
    label_w = std::max(label_w, m.label.size());
    if (m.decided()) name_w = std::max(name_w, m.winner().size());
  }
  char line[256];
  os << "winner per metric (ties: within " << v.tie_pct << "% or run-to-run noise):\n";
  for (const auto& m : v.metrics) {
    const std::string note = m.better == Better::Higher ? " (higher is better)" : "";
    if (m.decided()) {
      const Ranked& runner = m.ranking[1];
      std::snprintf(line, sizeof(line), "  %-*s  %-*s  %s better than %s%s\n",
                    static_cast<int>(label_w), m.label.c_str(), static_cast<int>(name_w),
                    m.winner().c_str(), fmt_ratio(runner.ratio).c_str(),
                    runner.name.c_str(), note.c_str());
    } else {
      std::snprintf(line, sizeof(line), "  %-*s  tie: %s\n", static_cast<int>(label_w),
                    m.label.c_str(), join_names(m.tied).c_str());
    }
    os << line;
  }
  if (v.overall_decided()) {
    std::snprintf(line, sizeof(line), "overall: %s (%s better than %s, geometric mean)\n",
                  v.overall[0].name.c_str(), fmt_ratio(v.overall[1].ratio).c_str(),
                  v.overall[1].name.c_str());
  } else {
    std::snprintf(line, sizeof(line), "overall: tie: %s\n",
                  join_names(v.overall_tied).c_str());
  }
  os << line;
}

inline void json_names(JsonWriter& w, const std::vector<std::string>& names) {
  w.begin_array(JsonWriter::Compact);
  for (const auto& n : names) w.value(n);
  w.end_array();
}

inline void json_ranking(JsonWriter& w, const std::vector<Ranked>& ranking) {
  w.begin_array(JsonWriter::Compact);
  for (const auto& r : ranking) {
    w.begin_object().field("name", r.name).field("ratio", r.ratio).end_object();
  }
  w.end_array();
}

/// Verdict as a JSON object. Fields: tie_pct; metrics{name: {better, winner,
/// tied, ranking}}; overall{winner, tied, ranking}. "winner" is "" when tied
/// or with fewer than two candidates; ranking ratios are >= 1 (1 = best).
inline void write_verdict_json(JsonWriter& w, const Verdict& v) {
  w.begin_object();
  w.field("tie_pct", v.tie_pct);
  w.key("metrics").begin_object();
  for (const auto& m : v.metrics) {
    w.key(m.metric).begin_object(JsonWriter::Compact);
    w.field("better", m.better == Better::Lower ? "lower" : "higher");
    w.field("winner", m.decided() ? m.winner() : std::string());
    w.key("tied");
    json_names(w, m.tied);
    w.key("ranking");
    json_ranking(w, m.ranking);
    w.end_object();
  }
  w.end_object();
  w.key("overall").begin_object(JsonWriter::Compact);
  w.field("winner", v.overall_decided() ? v.overall.front().name : std::string());
  w.key("tied");
  json_names(w, v.overall_tied);
  w.key("ranking");
  json_ranking(w, v.overall);
  w.end_object();
  w.end_object();
}

// --- aggregation over many inputs -----------------------------------------------

/// How one candidate fared across several judged inputs (archives, images).
struct AggregateEntry {
  std::string name;
  int inputs = 0;       // inputs in which it was ranked
  int wins = 0;         // inputs it won outright overall
  int ties = 0;         // inputs in which it tied for best overall
  double overall = 1.0;           // geometric mean of its overall ratios
  std::vector<double> per_metric; // same, per metric (aligned with `metrics`)
};

struct AggregateResult {
  int inputs = 0;   // comparable inputs aggregated
  int skipped = 0;  // inputs with fewer than two ranked candidates
  double tie_pct = 5.0;
  std::vector<MetricSpec> metrics;
  std::vector<AggregateEntry> entries;  // best (lowest overall) first
  std::vector<std::string> tied;        // within tie_pct of the best overall

  bool empty() const { return entries.empty(); }
  bool decided() const { return entries.size() > 1 && tied.size() == 1; }
};

/// Accumulates verdicts. Ratios are geometric-meaned, so one input where a
/// candidate is 10x slower and one where it is 10x faster cancel out.
class Aggregator {
 public:
  Aggregator(std::vector<MetricSpec> metrics, double tie_pct)
      : metrics_(std::move(metrics)), tie_pct_(tie_pct) {}

  void add(const Verdict& v) {
    if (v.overall.size() < 2) {  // nothing was compared on this input
      ++skipped_;
      return;
    }
    ++inputs_;
    for (const auto& r : v.overall) {
      Acc& a = acc(r.name);
      ++a.inputs;
      a.log_overall += std::log(r.ratio);
    }
    if (v.overall_decided()) {
      ++acc(v.overall.front().name).wins;
    } else {
      for (const auto& n : v.overall_tied) ++acc(n).ties;
    }
    for (std::size_t m = 0; m < v.metrics.size() && m < metrics_.size(); ++m) {
      for (const auto& r : v.metrics[m].ranking) {
        acc(r.name).log_metric[m] += std::log(r.ratio);
      }
    }
  }

  AggregateResult result() const {
    AggregateResult out;
    out.inputs = inputs_;
    out.skipped = skipped_;
    out.tie_pct = tie_pct_;
    out.metrics = metrics_;
    for (const Acc& a : accs_) {
      AggregateEntry e;
      e.name = a.name;
      e.inputs = a.inputs;
      e.wins = a.wins;
      e.ties = a.ties;
      e.overall = std::exp(a.log_overall / a.inputs);
      for (double l : a.log_metric) e.per_metric.push_back(std::exp(l / a.inputs));
      out.entries.push_back(std::move(e));
    }
    std::stable_sort(out.entries.begin(), out.entries.end(),
                     [](const auto& x, const auto& y) { return x.overall < y.overall; });
    if (!out.entries.empty()) {
      const double limit = out.entries.front().overall * (1.0 + tie_pct_ / 100.0);
      for (const auto& e : out.entries) {
        if (e.overall <= limit) out.tied.push_back(e.name);
      }
    }
    return out;
  }

 private:
  struct Acc {
    std::string name;
    int inputs = 0, wins = 0, ties = 0;
    double log_overall = 0;
    std::vector<double> log_metric;
  };

  Acc& acc(const std::string& name) {
    for (Acc& a : accs_) {
      if (a.name == name) return a;
    }
    accs_.push_back(Acc{name, 0, 0, 0, 0.0, std::vector<double>(metrics_.size(), 0.0)});
    return accs_.back();
  }

  std::vector<MetricSpec> metrics_;
  double tie_pct_;
  int inputs_ = 0;
  int skipped_ = 0;
  std::vector<Acc> accs_;
};

/// "Summary over N archives" table plus the overall line. `noun` is plural
/// ("archives", "images"); metric columns use the specs' labels.
inline void print_aggregate(std::ostream& os, const AggregateResult& a,
                            const std::string& noun) {
  os << "Summary over " << a.inputs << " comparable " << noun;
  if (a.skipped) {
    os << " (" << a.skipped << " skipped: fewer than two candidates measurable)";
  }
  os << "\n";
  if (a.empty()) {
    os << "  nothing to summarize\n";
    return;
  }
  os << "Cost vs the best per input, geometric mean; 1.00x = best everywhere.\n";
  std::vector<Column> cols = {{"variant", Align::Left}, {noun, Align::Right},
                              {"wins", Align::Right}, {"ties", Align::Right},
                              {"overall", Align::Right}};
  for (const auto& m : a.metrics) {
    cols.push_back({!m.short_label.empty() ? m.short_label
                    : !m.label.empty()      ? m.label
                                            : m.name,
                    Align::Right});
  }
  TextTable table(std::move(cols));
  for (const auto& e : a.entries) {
    std::vector<std::string> row = {e.name, std::to_string(e.inputs) + "/" + std::to_string(a.inputs),
                                    std::to_string(e.wins), std::to_string(e.ties),
                                    fmt_ratio(e.overall)};
    for (double r : e.per_metric) row.push_back(fmt_ratio(r));
    table.add_row(std::move(row));
  }
  table.print(os, "  ");
  if (a.decided()) {
    os << "overall: " << a.entries[0].name << " ("
       << fmt_ratio(a.entries[1].overall / a.entries[0].overall) << " better than "
       << a.entries[1].name << ", geometric mean over " << a.inputs << ' ' << noun << ")\n";
  } else {
    os << "overall: tie: " << join_names(a.tied) << "\n";
  }
}

inline void write_aggregate_json(JsonWriter& w, const AggregateResult& a) {
  w.begin_object();
  w.field("inputs", a.inputs);
  w.field("skipped", a.skipped);
  w.field("tie_pct", a.tie_pct);
  w.field("winner", a.decided() ? a.entries.front().name : std::string());
  w.key("tied");
  json_names(w, a.tied);
  w.key("variants").begin_array();
  for (const auto& e : a.entries) {
    w.begin_object(JsonWriter::Compact);
    w.field("name", e.name);
    w.field("inputs", e.inputs);
    w.field("wins", e.wins);
    w.field("ties", e.ties);
    w.field("overall_ratio", e.overall);
    w.key("metric_ratio").begin_object(JsonWriter::Compact);
    for (std::size_t m = 0; m < a.metrics.size(); ++m) w.field(a.metrics[m].name, e.per_metric[m]);
    w.end_object();
    w.end_object();
  }
  w.end_array();
  w.end_object();
}

}  // namespace gp
