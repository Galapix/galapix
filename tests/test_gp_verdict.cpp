// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gp_verdict.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>

namespace {

int g_fails = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_fails;
  }
}

bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

}  // namespace

int main() {
  using gp::Better;

  const std::vector<gp::MetricSpec> lower2 = {{"toc_ms", Better::Lower},
                                              {"extract_ms", Better::Lower}};

  // Clear winner on both metrics.
  {
    auto v = gp::judge(lower2, {{"a", {1.0, 10.0}}, {"b", {2.0, 30.0}}}, 5.0);
    expect(v.metrics.size() == 2, "two metric verdicts");
    expect(v.metrics[0].decided() && v.metrics[0].winner() == "a", "a wins toc");
    expect(near(v.metrics[0].ranking[1].ratio, 2.0), "b is 2x on toc");
    expect(near(v.metrics[1].ranking[1].ratio, 3.0), "b is 3x on extract");
    expect(v.overall_decided() && v.overall[0].name == "a", "a wins overall");
    expect(near(v.overall[0].ratio, 1.0), "best overall normalized to 1");
    expect(near(v.overall[1].ratio, std::sqrt(6.0)), "geomean of 2 and 3");
  }

  // Tie band: 3% apart with 5% band -> tie; 3% apart with 1% band -> decided.
  {
    auto v = gp::judge({{"ms", Better::Lower}}, {{"a", {1.00}}, {"b", {1.03}}}, 5.0);
    expect(!v.metrics[0].decided(), "within band is a tie");
    expect(v.metrics[0].tied.size() == 2, "both tied");
    expect(!v.overall_decided(), "overall tie");
    auto w = gp::judge({{"ms", Better::Lower}}, {{"a", {1.00}}, {"b", {1.03}}}, 1.0);
    expect(w.metrics[0].decided(), "outside band is decided");
  }

  // Noise-aware ties: 17% apart, but b's fastest run beats a's median.
  {
    gp::Candidate a{"a", {1.00}, {{0.95, 1.40}}};
    gp::Candidate b{"b", {1.17}, {{0.98, 1.30}}};
    auto v = gp::judge({{"ms", Better::Lower}}, {a, b}, 5.0);
    expect(!v.metrics[0].decided(), "overlap within noise is a tie");
    // b's fastest run (1.05) is slower than a's median (1.00): decided.
    gp::Candidate b2{"b", {1.17}, {{1.05, 1.30}}};
    auto w = gp::judge({{"ms", Better::Lower}}, {a, b2}, 5.0);
    expect(w.metrics[0].decided() && w.metrics[0].winner() == "a", "clear gap decided");
    // Zero-width spread (deterministic bytes) is decided by the band alone.
    gp::Candidate c1{"c1", {100.0}, {{100.0, 100.0}}};
    gp::Candidate c2{"c2", {110.0}, {{110.0, 110.0}}};
    auto z = gp::judge({{"bytes", Better::Lower}}, {c1, c2}, 5.0);
    expect(z.metrics[0].decided(), "zero spread decided by band");
    // Higher-is-better: b's best run reaches a's median -> tie.
    gp::Candidate h1{"h1", {40.0}, {{39.0, 41.0}}};
    gp::Candidate h2{"h2", {36.0}, {{35.0, 40.5}}};
    auto h = gp::judge({{"psnr", Better::Higher}}, {h1, h2}, 5.0);
    expect(!h.metrics[0].decided(), "higher-better noise tie");
    // Mismatched spread arity -> candidate dropped.
    gp::Candidate bad{"bad", {1.0, 2.0}, {{1.0, 1.0}}};
    auto d = gp::judge(lower2, {bad});
    expect(d.empty(), "spread arity mismatch dropped");
  }

  // Higher-is-better metric.
  {
    auto v = gp::judge({{"psnr", Better::Higher}}, {{"a", {30.0}}, {"b", {40.0}}}, 5.0);
    expect(v.metrics[0].winner() == "b", "higher wins");
    expect(near(v.metrics[0].ranking[1].ratio, 40.0 / 30.0), "higher ratio");
  }

  // Split decision: a wins one metric, b the other; overall by geomean.
  {
    auto v = gp::judge(lower2, {{"a", {1.0, 4.0}}, {"b", {2.0, 1.0}}}, 5.0);
    expect(v.metrics[0].winner() == "a" && v.metrics[1].winner() == "b", "split");
    // a: sqrt(1*4)=2, b: sqrt(2*1)=1.414 -> b wins overall
    expect(v.overall[0].name == "b", "geomean picks b");
    expect(near(v.overall[1].ratio, 2.0 / std::sqrt(2.0)), "normalized score");
  }

  // Invalid candidates are excluded, never winners.
  {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    auto v = gp::judge(lower2,
                       {{"ok", {1.0, 1.0}}, {"broken", {nan, 0.0}}, {"short", {0.5}}});
    expect(v.overall.size() == 1 && v.overall[0].name == "ok", "only valid candidate");
    std::ostringstream os;
    gp::print_verdict(os, v);
    expect(os.str().find("only one valid candidate") != std::string::npos,
           "single-candidate message");
  }

  // Zero values do not produce infinities.
  {
    auto v = gp::judge({{"ms", Better::Lower}}, {{"a", {0.0}}, {"b", {1.0}}});
    expect(std::isfinite(v.overall[1].ratio), "zero floor keeps ratios finite");
  }

  // Empty input.
  {
    auto v = gp::judge(lower2, {});
    expect(v.empty(), "no candidates -> empty");
    std::ostringstream os;
    gp::JsonWriter w(os);
    gp::write_verdict_json(w, v);
    expect(os.str().find("\"winner\": \"\"") != std::string::npos, "empty winner json");
  }

  // JSON shape smoke.
  {
    auto v = gp::judge(lower2, {{"a", {1.0, 10.0}}, {"b", {2.0, 30.0}}});
    std::ostringstream os;
    gp::JsonWriter w(os);
    gp::write_verdict_json(w, v);
    const std::string s = os.str();
    expect(s.find("\"toc_ms\": {\"better\": \"lower\", \"winner\": \"a\"") != std::string::npos,
           "metric json");
    expect(s.find("\"overall\": {\"winner\": \"a\"") != std::string::npos, "overall json");
  }

  // overall_ratio lookup and labels.
  {
    auto v = gp::judge({{"toc_ms", Better::Lower, "table of contents"}},
                       {{"a", {1.0}}, {"b", {2.0}}});
    expect(v.overall_ratio("a") && near(*v.overall_ratio("a"), 1.0), "ratio of best");
    expect(v.overall_ratio("b") && near(*v.overall_ratio("b"), 2.0), "ratio of runner-up");
    expect(!v.overall_ratio("zzz"), "unknown candidate");
    expect(v.metrics[0].label == "table of contents", "label carried");
    auto u = gp::judge({{"toc_ms", Better::Lower}}, {{"a", {1.0}}, {"b", {2.0}}});
    expect(u.metrics[0].label == "toc_ms", "label defaults to name");
    std::ostringstream os;
    gp::print_verdict(os, v);
    expect(os.str().find("table of contents") != std::string::npos &&
               os.str().find("2.00x better than b") != std::string::npos,
           "text uses label and ratio");
    expect(os.str().find("lower is better") == std::string::npos, "no noise for lower");
  }

  // Aggregation.
  {
    const std::vector<gp::MetricSpec> m = {{"ms", Better::Lower, "time"}};
    gp::Aggregator agg(m, 5.0);
    // Input 1: a 2x faster than b. Input 2: b 2x faster than a. -> cancel out.
    agg.add(gp::judge(m, {{"a", {1.0}}, {"b", {2.0}}}));
    agg.add(gp::judge(m, {{"a", {2.0}}, {"b", {1.0}}}));
    auto r = agg.result();
    expect(r.inputs == 2 && r.skipped == 0, "two inputs");
    expect(r.entries.size() == 2 && near(r.entries[0].overall, r.entries[1].overall),
           "geomean cancels 2x and 0.5x");
    expect(!r.decided() && r.tied.size() == 2, "cancelled -> tie");
    expect(r.entries[0].wins + r.entries[1].wins == 2 && r.entries[0].wins == 1,
           "one outright win each");

    // A third input where a wins clearly decides the aggregate.
    agg.add(gp::judge(m, {{"a", {1.0}}, {"b", {4.0}}}));
    r = agg.result();
    expect(r.decided() && r.entries[0].name == "a", "a wins overall");
    expect(r.entries[0].wins == 2 && r.entries[1].wins == 1, "win counts");
    expect(near(r.entries[0].per_metric[0], std::exp((0.0 + std::log(2.0) + 0.0) / 3.0)),
           "per-metric geomean");

    // Ties are counted separately; non-comparable inputs are skipped.
    gp::Aggregator t(m, 5.0);
    t.add(gp::judge(m, {{"a", {1.00}}, {"b", {1.01}}}));
    t.add(gp::judge(m, {{"only", {1.0}}}));
    t.add(gp::judge(m, {}));
    auto tr = t.result();
    expect(tr.inputs == 1 && tr.skipped == 2, "skipped inputs counted");
    expect(tr.entries[0].ties == 1 && tr.entries[0].wins == 0, "tie counted");

    // A candidate missing from some inputs is averaged over those it appeared in.
    gp::Aggregator p(m, 5.0);
    p.add(gp::judge(m, {{"a", {1.0}}, {"b", {2.0}}, {"c", {3.0}}}));
    p.add(gp::judge(m, {{"a", {1.0}}, {"b", {2.0}}}));
    auto pr = p.result();
    int c_inputs = 0;
    for (const auto& e : pr.entries) {
      if (e.name == "c") c_inputs = e.inputs;
    }
    expect(c_inputs == 1, "partial candidate counted once");

    std::ostringstream os;
    gp::print_aggregate(os, r, "archives");
    const std::string text = os.str();
    expect(text.find("Summary over 3 comparable archives") != std::string::npos, "summary header");
    expect(text.find("overall: a (") != std::string::npos, "summary overall line");
    std::ostringstream js;
    gp::JsonWriter w(js);
    gp::write_aggregate_json(w, r);
    expect(js.str().find("\"winner\": \"a\"") != std::string::npos &&
               js.str().find("\"metric_ratio\": {\"ms\"") != std::string::npos,
           "aggregate json");
    std::ostringstream empty;
    gp::print_aggregate(empty, gp::Aggregator(m, 5.0).result(), "images");
    expect(empty.str().find("nothing to summarize") != std::string::npos, "empty aggregate");
  }

  if (g_fails) {
    std::cerr << g_fails << " failure(s)\n";
    return 1;
  }
  std::cout << "ok: gp_verdict\n";
  return 0;
}
