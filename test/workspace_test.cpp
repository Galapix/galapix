// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

#include <thumtoo/lod/tile_backend.hpp>

#include "galapix/components.hpp"
#include "galapix/image_opener.hpp"
#include "galapix/workspace.hpp"
#include "job/job_manager.hpp"

using namespace galapix;

namespace {

/** A provider whose tiles never arrive, enough for size and scale */
class FakeProvider final : public TileProvider
{
public:
  explicit FakeProvider(Size size) : m_size(size) {}

  std::shared_ptr<thumtoo::lod::TileBackend> create_backend() override
  {
    struct Backend final : thumtoo::lod::TileBackend
    {
      void fetch(std::vector<thumtoo::lod::FetchRequest>, ResultFn) override {}
      void cancel(std::vector<thumtoo::lod::TileKey> const&) override {}
    };
    return std::make_shared<Backend>();
  }

  int get_max_scale() const override { return 4; }
  int get_tilesize() const override { return 256; }
  Size get_size() const override { return m_size; }

private:
  Size m_size;
};

URL
file_url(std::string const& name)
{
  return URL::from_filename("/tmp/galapix-workspace-test/" + name);
}

} // namespace

class WorkspaceTest : public ::testing::Test
{
protected:
  WorkspaceTest() : jobs(1), opener(jobs, nullptr) { jobs.start_thread(); }
  ~WorkspaceTest() override
  {
    jobs.stop_thread();
    jobs.join_thread();
  }

  JobManager jobs;
  ImageOpener opener;
};

TEST_F(WorkspaceTest, add_and_pick)
{
  Workspace ws;
  entt::entity const a = ws.add_image(file_url("a.jpg"), nullptr);
  entt::entity const b = ws.add_image(file_url("b.jpg"), std::make_shared<FakeProvider>(Size(1000, 500)));

  ASSERT_EQ(ws.images().size(), 2u);
  EXPECT_EQ(ws.registry().get<ImageSize>(a).width, 256);
  EXPECT_FALSE(ws.registry().all_of<Tiles>(a));
  EXPECT_EQ(ws.registry().get<ImageSize>(b).width, 1000);
  EXPECT_TRUE(ws.registry().all_of<Tiles>(b));

  // b is on top of a at the origin, both are centered there
  EXPECT_EQ(ws.get_image(Vector2f(0.0f, 0.0f)), b);
  EXPECT_TRUE(ws.get_image(Vector2f(5000.0f, 5000.0f)) == entt::null);
}

TEST_F(WorkspaceTest, layout_writes_transforms_in_order)
{
  Workspace ws;
  std::vector<entt::entity> images;
  for (int i = 0; i < 6; ++i) {
    images.push_back(ws.add_image(file_url("img" + std::to_string(i) + ".jpg"),
                                  std::make_shared<FakeProvider>(Size(2000, 1000))));
  }

  ws.layout_spiral();

  // Spiral: every image normalized to 1000 wide, on distinct 1024 grid cells
  for (entt::entity const image : images) {
    EXPECT_FLOAT_EQ(ws.registry().get<Transform>(image).scale, 0.5f);
  }
  EXPECT_EQ(ws.registry().get<Transform>(images[0]).pos, Vector2f(1024.0f, 0.0f));
  EXPECT_EQ(ws.registry().get<Transform>(images[1]).pos, Vector2f(1024.0f, 1024.0f));

  // overlap free afterwards
  ws.layout_random();
  ws.solve_overlaps();
  for (size_t i = 0; i < images.size(); ++i) {
    for (size_t j = i + 1; j < images.size(); ++j) {
      Rectf const ri = ws.get_image_rect(images[i]);
      Rectf const rj = ws.get_image_rect(images[j]);
      EXPECT_FALSE(geom::intersects(ri, rj)) << i << " " << j;
    }
  }
}

TEST_F(WorkspaceTest, selection)
{
  Workspace ws;
  entt::entity const a = ws.add_image(file_url("a.jpg"), nullptr);
  entt::entity const b = ws.add_image(file_url("b.jpg"), nullptr);
  entt::entity const c = ws.add_image(file_url("c.jpg"), nullptr);
  ws.registry().get<Transform>(a).pos = Vector2f(0.0f, 0.0f);
  ws.registry().get<Transform>(b).pos = Vector2f(1000.0f, 0.0f);
  ws.registry().get<Transform>(c).pos = Vector2f(5000.0f, 0.0f);

  EXPECT_TRUE(ws.selection_empty());
  ws.select_images(ws.get_images(Rectf(-200.0f, -200.0f, 1200.0f, 200.0f)));
  ASSERT_EQ(ws.get_selection(), (std::vector<entt::entity>{a, b}));
  EXPECT_EQ(ws.get_selection_center(), Vector2f(500.0f, 0.0f));

  ws.move_selection(Vector2f(0.0f, 100.0f));
  EXPECT_EQ(ws.registry().get<Transform>(a).pos, Vector2f(0.0f, 100.0f));
  EXPECT_EQ(ws.registry().get<Transform>(c).pos, Vector2f(5000.0f, 0.0f));

  ws.scale_selection(2.0f);
  EXPECT_FLOAT_EQ(ws.registry().get<Transform>(a).scale, 2.0f);
  EXPECT_EQ(ws.registry().get<Transform>(a).pos, Vector2f(-500.0f, 100.0f));

  ws.deselect(b);
  EXPECT_EQ(ws.get_selection(), (std::vector<entt::entity>{a}));

  ws.delete_selection();
  EXPECT_FALSE(ws.valid(a));
  EXPECT_EQ(ws.images(), (std::vector<entt::entity>{b, c}));

  ws.select(c);
  ws.isolate_selection();
  EXPECT_EQ(ws.images(), (std::vector<entt::entity>{c}));
  EXPECT_TRUE(ws.selection_empty());
}

TEST_F(WorkspaceTest, save_load_roundtrip)
{
  Workspace ws;
  entt::entity const a = ws.add_image(file_url("a \"quoted\".jpg"), nullptr);
  entt::entity const b = ws.add_image(file_url("b.jpg"), nullptr);
  ws.registry().get<Transform>(a) = Transform{Vector2f(10.0f, 20.0f), 0.25f, 0.0f};
  ws.registry().get<Transform>(b) = Transform{Vector2f(-30.0f, 40.0f), 2.0f, 0.0f};

  std::filesystem::create_directories("/tmp/galapix-workspace-test");
  std::string const filename = "/tmp/galapix-workspace-test/roundtrip.galapix";
  {
    std::ofstream out(filename);
    ws.save(out);
  }

  Workspace loaded;
  loaded.load(filename, opener);
  ASSERT_EQ(loaded.images().size(), 2u);

  entt::entity const la = loaded.images()[0];
  entt::entity const lb = loaded.images()[1];
  EXPECT_EQ(loaded.get_url(la).str(), ws.get_url(a).str());
  EXPECT_EQ(loaded.get_url(lb).str(), ws.get_url(b).str());
  EXPECT_EQ(loaded.registry().get<Transform>(la).pos, Vector2f(10.0f, 20.0f));
  EXPECT_FLOAT_EQ(loaded.registry().get<Transform>(lb).scale, 2.0f);

  // The saved scale refers to the native size: when the size probe
  // attaches the provider later, it must stay.
  EXPECT_TRUE(loaded.registry().all_of<ScaleIsNative>(lb));
  loaded.set_tile_provider(lb, std::make_shared<FakeProvider>(Size(4000, 3000)));
  EXPECT_FLOAT_EQ(loaded.registry().get<Transform>(lb).scale, 2.0f);
  EXPECT_FALSE(loaded.registry().all_of<ScaleIsNative>(lb));
}

TEST_F(WorkspaceTest, provider_keeps_displayed_size)
{
  Workspace ws;
  entt::entity const a = ws.add_image(file_url("a.jpg"), nullptr);
  ws.registry().get<Transform>(a).scale = 2.0f;  // 512 wide on screen

  ws.set_tile_provider(a, std::make_shared<FakeProvider>(Size(1024, 768)));
  EXPECT_FLOAT_EQ(ws.get_image_rect(a).width(), 512.0f);
}

/* EOF */
