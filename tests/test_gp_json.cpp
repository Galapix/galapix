// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "gp_json.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

int g_fails = 0;

void expect_eq(const std::string& got, const std::string& want, const char* msg) {
  if (got != want) {
    std::cerr << "FAIL: " << msg << "\n--- got:\n" << got << "--- want:\n" << want;
    ++g_fails;
  }
}

template <typename F>
void expect_throws(F f, const char* msg) {
  std::ostringstream os;
  gp::JsonWriter w(os);
  try {
    f(w);
  } catch (const std::logic_error&) {
    return;
  }
  std::cerr << "FAIL: expected logic_error: " << msg << "\n";
  ++g_fails;
}

}  // namespace

int main() {
  {
    std::ostringstream os;
    gp::JsonWriter w(os);
    w.begin_object();
    w.field("tool", "gp").field("n", 3).field("big", std::uint64_t{18446744073709551615ULL});
    w.key("rows").begin_array();
    w.begin_object(gp::JsonWriter::Compact).field("q", 80).field("ms", 1.5).end_object();
    w.begin_object(gp::JsonWriter::Compact).field("q", 90).field("ok", true).end_object();
    w.end_array();
    w.key("empty_obj").begin_object().end_object();
    w.key("empty_arr").begin_array().end_array();
    w.key("nested").begin_object().key("list").begin_array(gp::JsonWriter::Compact);
    w.value("a\"b").value(nullptr).begin_object().field("x", -1).end_object();
    w.end_array().end_object();
    w.end_object();
    expect_eq(os.str(),
              "{\n"
              "  \"tool\": \"gp\",\n"
              "  \"n\": 3,\n"
              "  \"big\": 18446744073709551615,\n"
              "  \"rows\": [\n"
              "    {\"q\": 80, \"ms\": 1.5},\n"
              "    {\"q\": 90, \"ok\": true}\n"
              "  ],\n"
              "  \"empty_obj\": {},\n"
              "  \"empty_arr\": [],\n"
              "  \"nested\": {\n"
              "    \"list\": [\"a\\\"b\", null, {\"x\": -1}]\n"
              "  }\n"
              "}\n",
              "nested document");
    if (!w.complete()) {
      std::cerr << "FAIL: writer not complete\n";
      ++g_fails;
    }
  }
  {
    std::ostringstream os;
    gp::JsonWriter w(os);
    w.begin_array(gp::JsonWriter::Compact);
    w.value(std::numeric_limits<double>::quiet_NaN());
    w.value(std::numeric_limits<double>::infinity());
    w.value(0.000123456789);
    w.end_array();
    expect_eq(os.str(), "[null, null, 0.000123457]\n", "non-finite -> null, %g precision");
  }
  {
    std::ostringstream os;
    gp::JsonWriter w(os);
    w.value(42);
    expect_eq(os.str(), "42\n", "scalar root");
  }

  expect_throws([](gp::JsonWriter& w) { w.begin_object().value(1); }, "value without key");
  expect_throws([](gp::JsonWriter& w) { w.begin_array().key("k"); }, "key in array");
  expect_throws([](gp::JsonWriter& w) { w.begin_object().end_array(); }, "mismatched close");
  expect_throws([](gp::JsonWriter& w) { w.begin_object().key("k").end_object(); },
                "close with pending key");
  expect_throws([](gp::JsonWriter& w) { w.value(1).value(2); }, "two roots");

  if (g_fails) {
    std::cerr << g_fails << " failure(s)\n";
    return 1;
  }
  std::cout << "ok: gp_json\n";
  return 0;
}
