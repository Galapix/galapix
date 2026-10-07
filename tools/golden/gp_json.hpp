// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Minimal streaming JSON writer for the golden tools.
//
// Hand-concatenated JSON with fixed indents cannot nest one document inside
// another (batch output wraps per-file documents), and every comma is a
// chance for invalid output. The writer tracks nesting, commas and
// indentation; callers only say what to write:
//
//   gp::JsonWriter w(std::cout);
//   w.begin_object();
//   w.key("tool").value("thumtoo-gp-tile");
//   w.key("rows").begin_array();
//   w.begin_object(gp::JsonWriter::Compact).key("quality").value(80).end_object();
//   w.end_array();
//   w.end_object();   // final newline written when the outermost scope closes
//
// Compact scopes print on one line (used for table-like rows). Non-finite
// doubles are written as null (JSON has no NaN/Infinity).

#pragma once

#include "gp_common.hpp"

#include <cmath>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace gp {

class JsonWriter {
 public:
  enum Style { Pretty, Compact };

  explicit JsonWriter(std::ostream& os, int indent = 2) : os_(os), indent_(indent) {}
  JsonWriter(const JsonWriter&) = delete;
  JsonWriter& operator=(const JsonWriter&) = delete;

  JsonWriter& begin_object(Style style = Pretty) { return open('{', style, false); }
  JsonWriter& end_object() { return close('}', false); }
  JsonWriter& begin_array(Style style = Pretty) { return open('[', style, true); }
  JsonWriter& end_array() { return close(']', true); }

  JsonWriter& key(std::string_view k) {
    if (stack_.empty() || stack_.back().array || expect_value_) {
      throw std::logic_error("JsonWriter: key outside object");
    }
    separator();
    json_string(os_, k);
    os_ << ": ";
    expect_value_ = true;
    return *this;
  }

  JsonWriter& value(std::string_view s) {
    prefix();
    json_string(os_, s);
    return done();
  }
  JsonWriter& value(const char* s) { return value(std::string_view(s)); }
  JsonWriter& value(const std::string& s) { return value(std::string_view(s)); }
  JsonWriter& value(bool b) {
    prefix();
    os_ << (b ? "true" : "false");
    return done();
  }
  JsonWriter& value(std::nullptr_t) {
    prefix();
    os_ << "null";
    return done();
  }
  JsonWriter& value(double d) {
    prefix();
    if (std::isfinite(d)) {
      std::ostringstream tmp;  // default %g-style, 6 significant digits
      tmp << d;
      os_ << tmp.str();
    } else {
      os_ << "null";
    }
    return done();
  }
  template <typename T,
            std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
  JsonWriter& value(T n) {
    prefix();
    if constexpr (std::is_signed_v<T>) {
      os_ << static_cast<long long>(n);
    } else {
      os_ << static_cast<unsigned long long>(n);
    }
    return done();
  }

  /// key(k).value(v) in one call.
  template <typename T>
  JsonWriter& field(std::string_view k, const T& v) {
    return key(k).value(v);
  }

  /// All scopes closed and nothing pending?
  bool complete() const { return stack_.empty() && wrote_root_; }

 private:
  struct Scope {
    bool array = false;
    bool compact = false;
    int count = 0;
  };

  bool compact_now() const { return !stack_.empty() && stack_.back().compact; }

  void newline_indent(std::size_t depth) {
    os_ << '\n';
    for (std::size_t i = 0; i < depth * static_cast<std::size_t>(indent_); ++i) os_ << ' ';
  }

  // Comma + line break before an element of the current scope.
  void separator() {
    Scope& s = stack_.back();
    if (s.count++ > 0) os_ << (s.compact ? ", " : ",");
    if (!s.compact) newline_indent(stack_.size());
  }

  // Before any value (scalar or container).
  void prefix() {
    if (stack_.empty()) {
      if (wrote_root_) throw std::logic_error("JsonWriter: second root value");
      return;
    }
    if (stack_.back().array) {
      separator();
    } else if (!expect_value_) {
      throw std::logic_error("JsonWriter: value without key");
    }
    expect_value_ = false;
  }

  JsonWriter& done() {
    if (stack_.empty()) finish_root();
    return *this;
  }

  void finish_root() {
    wrote_root_ = true;
    os_ << '\n';
  }

  JsonWriter& open(char brace, Style style, bool array) {
    prefix();
    os_ << brace;
    // A compact parent forces compact children (no line breaks inside a line).
    stack_.push_back(Scope{array, style == Compact || compact_now(), 0});
    return *this;
  }

  JsonWriter& close(char brace, bool array) {
    if (stack_.empty() || stack_.back().array != array || expect_value_) {
      throw std::logic_error("JsonWriter: mismatched close");
    }
    const Scope s = stack_.back();
    stack_.pop_back();
    if (s.count > 0 && !s.compact) newline_indent(stack_.size());
    os_ << brace;
    return done();
  }

  std::ostream& os_;
  int indent_;
  std::vector<Scope> stack_;
  bool expect_value_ = false;
  bool wrote_root_ = false;
};

}  // namespace gp
