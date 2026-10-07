// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <vector>

#include <wstdisplay/batch.hpp>
#include <wstdisplay/canvas.hpp>

using namespace wstdisplay;

namespace {

// Canvas never resolves handles, made up ones are enough
TextureId const tex_a{1, 1};
TextureId const tex_b{2, 1};

std::vector<Vertex> quad(float x, float y)
{
  return {
    {x, y, 0, 0, packed_white},
    {x + 1, y, 1, 0, packed_white},
    {x + 1, y + 1, 1, 1, packed_white},
    {x, y + 1, 0, 1, packed_white},
  };
}

} // namespace

TEST(BatchTest, same_texture_is_one_batch)
{
  Canvas canvas;
  for (int i = 0; i < 100; ++i) {
    canvas.draw_quads(tex_a, quad(static_cast<float>(i), 0));
  }

  BatchList batches;
  build_batches(canvas, batches);

  // consecutive draws with equal state are already merged when recorded
  EXPECT_EQ(canvas.get_commands().size(), 1u);
  ASSERT_EQ(batches.batches.size(), 1u);
  EXPECT_EQ(batches.batches[0].index_count, 600u);
  EXPECT_EQ(batches.vertices.size(), 400u);
}

TEST(BatchTest, alternating_textures_break_batches)
{
  Canvas canvas;
  for (int i = 0; i < 10; ++i) {
    canvas.draw_quads(i % 2 == 0 ? tex_a : tex_b, quad(static_cast<float>(i), 0));
  }

  BatchList batches;
  build_batches(canvas, batches);
  EXPECT_EQ(batches.batches.size(), 10u);
}

TEST(BatchTest, z_sort_is_stable_and_merges)
{
  Canvas canvas;
  canvas.set_z(1.0f);
  canvas.draw_quads(tex_a, quad(0, 0));
  canvas.set_z(0.0f);
  canvas.draw_quads(tex_b, quad(1, 0));
  canvas.set_z(1.0f);
  canvas.draw_quads(tex_a, quad(2, 0));
  canvas.set_z(0.0f);
  canvas.draw_quads(tex_b, quad(3, 0));

  BatchList batches;
  build_batches(canvas, batches);

  // sorted: b(z=0) b(z=0) a(z=1) a(z=1), equal neighbors merge
  ASSERT_EQ(batches.batches.size(), 2u);
  EXPECT_EQ(batches.batches[0].texture, tex_b);
  EXPECT_EQ(batches.batches[1].texture, tex_a);

  // submission order is kept within the same z
  EXPECT_EQ(batches.vertices[0].x, 1.0f);
  EXPECT_EQ(batches.vertices[4].x, 3.0f);
  EXPECT_EQ(batches.vertices[8].x, 0.0f);
  EXPECT_EQ(batches.vertices[12].x, 2.0f);
}

TEST(BatchTest, blend_and_clip_break_batches)
{
  Canvas canvas;
  canvas.draw_quads(tex_a, quad(0, 0));
  canvas.set_blend(Blend::Add);
  canvas.draw_quads(tex_a, quad(1, 0));
  canvas.save();
  canvas.clip(geom::frect(0, 0, 10, 10));
  canvas.draw_quads(tex_a, quad(2, 0));
  canvas.restore();

  BatchList batches;
  build_batches(canvas, batches);
  ASSERT_EQ(batches.batches.size(), 3u);
  EXPECT_EQ(batches.batches[0].clip, 0);
  EXPECT_EQ(batches.batches[2].clip, 1);
}

TEST(BatchTest, indices_stay_16_bit)
{
  Canvas canvas;
  // 40000 quads = 160000 vertices, more than 16-bit indices can address
  std::vector<Vertex> vertices;
  for (int i = 0; i < 40000; ++i) {
    auto q = quad(static_cast<float>(i), 0);
    vertices.insert(vertices.end(), q.begin(), q.end());
  }
  canvas.draw_quads(tex_a, vertices);

  BatchList batches;
  build_batches(canvas, batches);

  EXPECT_EQ(batches.vertices.size(), 160000u);
  EXPECT_GE(batches.batches.size(), 3u);

  uint32_t total_indices = 0;
  for (auto const& batch : batches.batches) {
    total_indices += batch.index_count;
    for (uint32_t i = 0; i < batch.index_count; ++i) {
      uint32_t const vertex = batch.base_vertex + batches.indices[batch.first_index + i];
      ASSERT_LT(vertex, batches.vertices.size());
    }
  }
  EXPECT_EQ(total_indices, 240000u);

  // every index still points at the right vertex
  for (auto const& batch : batches.batches) {
    for (uint32_t i = 0; i < batch.index_count; i += 6) {
      Vertex const& v = batches.vertices[batch.base_vertex + batches.indices[batch.first_index + i]];
      Vertex const& w = batches.vertices[batch.base_vertex + batches.indices[batch.first_index + i + 2]];
      EXPECT_EQ(v.x + 1.0f, w.x);
    }
  }
}

TEST(BatchTest, object_commands_pass_through)
{
  Canvas canvas;
  canvas.draw_quads(tex_a, quad(0, 0));
  canvas.translate(5, 0);
  canvas.draw(StaticBatchId{3, 1});
  canvas.draw_quads(tex_a, quad(1, 0));

  BatchList batches;
  build_batches(canvas, batches);
  ASSERT_EQ(batches.batches.size(), 3u);
  EXPECT_EQ(batches.batches[1].type, CommandType::StaticBatch);

  ObjectDraw const& object = canvas.get_objects()[batches.batches[1].object];
  EXPECT_EQ(object.static_batch, (StaticBatchId{3, 1}));
  EXPECT_EQ(object.matrix[3][0], 5.0f);
}

TEST(BatchTest, unordered_layer_groups_by_state)
{
  Canvas canvas;
  canvas.set_layer_unordered(1.0f);

  // interleaved in an ordered layer: one batch per draw
  for (int i = 0; i < 6; ++i) {
    canvas.draw_quads(i % 2 == 0 ? tex_a : tex_b, quad(static_cast<float>(i), 0));
  }

  // interleaved in an unordered layer: grouped by texture
  canvas.set_z(1.0f);
  for (int i = 0; i < 6; ++i) {
    canvas.draw_quads(i % 2 == 0 ? tex_b : tex_a, quad(static_cast<float>(10 + i), 0));
  }

  BatchList batches;
  build_batches(canvas, batches);
  ASSERT_EQ(batches.batches.size(), 8u);
  EXPECT_EQ(batches.batches[6].texture, tex_a);
  EXPECT_EQ(batches.batches[6].index_count, 18u);
  EXPECT_EQ(batches.batches[7].texture, tex_b);

  // draws with equal state keep their order
  EXPECT_EQ(batches.vertices[24].x, 11.0f);
  EXPECT_EQ(batches.vertices[28].x, 13.0f);
  EXPECT_EQ(batches.vertices[32].x, 15.0f);
  EXPECT_EQ(batches.vertices[36].x, 10.0f);
}

TEST(BatchTest, unordered_layer_keeps_z_order)
{
  Canvas canvas;
  canvas.set_layer_unordered(0.0f);
  canvas.set_z(1.0f);
  canvas.draw_quads(tex_a, quad(0, 0));
  canvas.set_z(0.0f);
  canvas.draw_quads(tex_b, quad(1, 0));
  canvas.draw_quads(tex_a, quad(2, 0));

  BatchList batches;
  build_batches(canvas, batches);

  // z = 0 regrouped by texture, z = 1 still drawn last
  ASSERT_EQ(batches.batches.size(), 3u);
  EXPECT_EQ(batches.batches[0].texture, tex_a);
  EXPECT_EQ(batches.batches[1].texture, tex_b);
  EXPECT_EQ(batches.batches[2].texture, tex_a);
  EXPECT_EQ(batches.vertices[0].x, 2.0f);
  EXPECT_EQ(batches.vertices[8].x, 0.0f);
}

/* EOF */
