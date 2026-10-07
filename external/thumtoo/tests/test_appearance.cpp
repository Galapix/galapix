// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/appearance.hpp"

#include <cstdio>
#include <filesystem>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static int g_failures = 0;

static void expect(bool cond, const char* msg)
{
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

int main()
{
  const fs::path root =
      fs::temp_directory_path() / ("thumtoo-appearance-test-" + std::to_string(::getpid()));
  fs::remove_all(root);

  {
    auto store = thumtoo::AppearanceStore::open(root);
    expect(store.valid(), "open store");
    expect(fs::exists(store.db_path()), "db file exists");

    const std::string id =
        "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    expect(!store.get(id).has_value(), "empty get");

    thumtoo::ContentAppearance app;
    app.content_quarter_turns = 1;
    app.content_h_flip = true;
    store.put(id, app);

    auto got = store.get(id);
    expect(got.has_value(), "get after put");
    if (got) {
      expect(got->content_quarter_turns == 1, "turns");
      expect(got->content_h_flip, "hflip");
      expect(!got->content_v_flip, "vflip");
    }

    // Bare hex also works.
    auto got2 = store.get(
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    expect(got2.has_value(), "get by bare hex");

    // Identity clears the row.
    store.put(id, thumtoo::ContentAppearance{});
    expect(!store.get(id).has_value(), "identity removes");

    // Crop row.
    thumtoo::ContentAppearance crop;
    crop.has_crop = true;
    crop.crop_x = 10;
    crop.crop_y = 20;
    crop.crop_w = 100;
    crop.crop_h = 80;
    crop.crop_source_w = 200;
    crop.crop_source_h = 160;
    store.put(id, crop);
    auto g3 = store.get(id);
    expect(g3.has_value() && g3->has_crop && g3->crop_w == 100, "crop stored");
  }

  // normalize_content_id
  expect(thumtoo::normalize_content_id("not-hex").empty(), "reject short");
  {
    const std::string page_id =
        "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef:page:3";
    expect(thumtoo::normalize_content_id(page_id) == page_id, "page-qualified id");
    expect(thumtoo::normalize_content_id(
               "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF:page:12")
               == "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef:page:12",
           "page id uppercase hex");
    expect(thumtoo::normalize_content_id(
               "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef:page:0")
               .empty(),
           "reject page 0");
  }
  expect(thumtoo::normalize_content_id("").empty(), "reject empty");
  const std::string full =
      "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
  expect(thumtoo::normalize_content_id(
             "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF")
             == full,
         "uppercase hex normalized");

  // Invalid id is a no-op
  {
    auto store = thumtoo::AppearanceStore::open(root);
    thumtoo::ContentAppearance bad;
    bad.content_h_flip = true;
    store.put("nope", bad);
    expect(!store.get("nope").has_value(), "invalid id not stored");
  }

  // Re-open durable.
  {
    auto store = thumtoo::AppearanceStore::open(root);
    const std::string id =
        "sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    auto got = store.get(id);
    expect(got.has_value() && got->has_crop, "durable reopen");
  }

  // Page-qualified content id round-trip
  {
    auto store = thumtoo::AppearanceStore::open(root);
    const std::string page_id =
        "sha256:dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd:page:2";
    thumtoo::ContentAppearance a;
    a.content_quarter_turns = 1;
    store.put(page_id, a);
    auto g = store.get(page_id);
    expect(g.has_value() && g->content_quarter_turns == 1, "page id put/get");
    store.put(page_id, thumtoo::ContentAppearance{});
    expect(!store.get(page_id).has_value(), "page id clear");
  }

  // biltoo-like cycle: rotate 90 three times → turns=3; fourth clears
  {
    auto store = thumtoo::AppearanceStore::open(root);
    const std::string id2 =
        "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    thumtoo::ContentAppearance cycle;
    for (int step = 1; step <= 3; ++step) {
      cycle.content_quarter_turns = step;
      store.put(id2, cycle);
      auto g = store.get(id2);
      expect(g.has_value() && g->content_quarter_turns == step, "cycle step");
    }
    cycle.content_quarter_turns = 4;  // must normalize to 0 → identity delete
    store.put(id2, cycle);
    expect(!store.get(id2).has_value(), "turns 4 clears row");

    // Flip + turns round-trip (simulates commitItemSessionEdit payload)
    thumtoo::ContentAppearance edit;
    edit.content_h_flip = true;
    edit.content_quarter_turns = 1;
    store.put(id2, edit);
    auto round = store.get(id2);
    expect(round.has_value() && round->content_h_flip
               && round->content_quarter_turns == 1,
           "flip+turn round-trip");
    // Reset to identity
    store.put(id2, thumtoo::ContentAppearance{});
    expect(!store.get(id2).has_value(), "reset clears");
  }

  // default_state_root respects XDG_STATE_HOME
  {
    const fs::path custom = fs::temp_directory_path() / ("thumtoo-state-" + std::to_string(::getpid()));
    fs::remove_all(custom);
    ::setenv("XDG_STATE_HOME", custom.string().c_str(), 1);
    const auto root2 = thumtoo::default_state_root();
    expect(root2 == custom / "thumtoo", "XDG_STATE_HOME/thumtoo");
    auto store = thumtoo::AppearanceStore::open();
    expect(store.valid(), "open default under XDG_STATE_HOME");
    expect(store.db_path().parent_path() == custom / "thumtoo", "db under state");
    fs::remove_all(custom);
  }

  fs::remove_all(root);

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "ok\n";
  return 0;
}
