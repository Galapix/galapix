// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <wstdisplay/canvas.hpp>
#include <wstdisplay/surface.hpp>

using namespace wstdisplay;

TEST(CanvasTest, transform_is_applied_on_record)
{
  Canvas canvas;
  canvas.save();
  canvas.translate(10, 20);
  canvas.scale(2, 2);
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 1, 1));
  canvas.restore();
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 1, 1));

  auto const& v = canvas.get_vertices();
  ASSERT_EQ(v.size(), 8u);
  EXPECT_FLOAT_EQ(v[0].x, 10.0f);
  EXPECT_FLOAT_EQ(v[0].y, 20.0f);
  EXPECT_FLOAT_EQ(v[2].x, 12.0f);
  EXPECT_FLOAT_EQ(v[2].y, 22.0f);
  EXPECT_FLOAT_EQ(v[4].x, 0.0f);
  EXPECT_FLOAT_EQ(v[6].x, 1.0f);
}

TEST(CanvasTest, rotate_is_clockwise_on_screen)
{
  Canvas canvas;
  canvas.rotate(90.0f);
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 1, 1));

  // (1, 0) rotated by 90 degrees with y pointing down ends up at (0, 1)
  auto const& v = canvas.get_vertices();
  EXPECT_NEAR(v[1].x, 0.0f, 1e-6f);
  EXPECT_NEAR(v[1].y, 1.0f, 1e-6f);
}

TEST(CanvasTest, restore_without_save_throws)
{
  Canvas canvas;
  EXPECT_THROW(canvas.restore(), std::logic_error);
}

TEST(CanvasTest, save_restore_covers_all_state)
{
  Canvas canvas;
  canvas.save();
  canvas.translate(5, 5);
  canvas.clip(geom::frect(0, 0, 10, 10));
  canvas.set_z(3.0f);
  canvas.set_blend(Blend::Add);
  canvas.set_material(MaterialId{1, 1});
  canvas.set_space(Space::Screen);
  canvas.restore();

  EXPECT_EQ(canvas.get_transform(), glm::mat3(1.0f));
  EXPECT_EQ(canvas.get_z(), 0.0f);
  EXPECT_EQ(canvas.get_blend(), Blend::Alpha);
  EXPECT_FALSE(canvas.get_material());
  EXPECT_EQ(canvas.get_space(), Space::World);

  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 1, 1));
  EXPECT_EQ(canvas.get_commands().back().clip, 0);
}

TEST(CanvasTest, scope_restores)
{
  Canvas canvas;
  {
    Canvas::Scope scope(canvas);
    canvas.set_z(7.0f);
  }
  EXPECT_EQ(canvas.get_z(), 0.0f);
}

TEST(CanvasTest, state_is_recorded_in_commands)
{
  Canvas canvas;
  canvas.set_z(2.0f);
  canvas.set_blend(Blend::Multiply);
  canvas.set_material(MaterialId{4, 2});
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 1, 1));

  ASSERT_EQ(canvas.get_commands().size(), 1u);
  auto const& cmd = canvas.get_commands()[0];
  EXPECT_EQ(cmd.z, 2.0f);
  EXPECT_EQ(cmd.blend, Blend::Multiply);
  EXPECT_EQ(cmd.material, (MaterialId{4, 2}));
}

TEST(CanvasTest, clip_rects_nest)
{
  Canvas canvas;
  canvas.save();
  canvas.clip(geom::frect(0, 0, 100, 100));
  canvas.clip(geom::frect(50, 50, 200, 200));
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 1, 1));
  canvas.restore();

  auto const& clips = canvas.get_clip_rects();
  ASSERT_EQ(clips.size(), 3u);
  EXPECT_EQ(clips[2].rect, geom::frect(50, 50, 100, 100));
  EXPECT_EQ(canvas.get_commands().back().clip, 2);
}

TEST(CanvasTest, clip_rects_follow_transform)
{
  Canvas canvas;
  canvas.translate(10, 10);
  canvas.clip(geom::frect(0, 0, 5, 5));
  EXPECT_EQ(canvas.get_clip_rects()[1].rect, geom::frect(10, 10, 15, 15));
}

TEST(CanvasTest, shapes_merge_into_one_command)
{
  Canvas canvas;
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 0, 0));
  canvas.fill_circle(geom::fpoint(5, 5), 2.0f, surf::Color(0, 1, 0));
  canvas.draw_line(geom::fpoint(0, 0), geom::fpoint(5, 5), surf::Color(0, 0, 1));
  canvas.draw_rounded_rect(geom::frect(0, 0, 10, 10), 3.0f, surf::Color(1, 1, 1));
  EXPECT_EQ(canvas.get_commands().size(), 1u);
}

TEST(CanvasTest, different_z_doesnt_merge)
{
  Canvas canvas;
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 0, 0));
  canvas.set_z(1.0f);
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 0, 0));
  EXPECT_EQ(canvas.get_commands().size(), 2u);
}

TEST(CanvasTest, null_surface_draws_nothing)
{
  Canvas canvas;
  canvas.draw(Surface(), geom::fpoint(0, 0));
  canvas.draw(Surface(), DrawParams());
  EXPECT_TRUE(canvas.empty());
}

TEST(CanvasTest, draw_params_effect_reaches_vertices)
{
  Canvas canvas;
  Surface const surface(TextureId{0, 1}, geom::isize(16, 16));
  canvas.draw(surface, DrawParams().set_effect(surf::Color(1, 1, 1, 1)));
  canvas.draw(surface, geom::fpoint(0, 0));

  auto const& v = canvas.get_vertices();
  ASSERT_EQ(v.size(), 8u);
  EXPECT_EQ(v[0].effect, packed_white);
  EXPECT_EQ(v[4].effect, packed_transparent);

  // different effects still share a command
  EXPECT_EQ(canvas.get_commands().size(), 1u);
}

TEST(CanvasTest, circle_segments_follow_size)
{
  Canvas small;
  small.fill_circle(geom::fpoint(0, 0), 2.0f, surf::Color(1, 1, 1));
  Canvas large;
  large.fill_circle(geom::fpoint(0, 0), 200.0f, surf::Color(1, 1, 1));
  Canvas scaled;
  scaled.scale(100.0f, 100.0f);
  scaled.fill_circle(geom::fpoint(0, 0), 2.0f, surf::Color(1, 1, 1));

  EXPECT_GE(small.get_vertices().size(), 8u);
  EXPECT_GT(large.get_vertices().size(), small.get_vertices().size());
  EXPECT_EQ(scaled.get_vertices().size(), large.get_vertices().size());
}

TEST(CanvasTest, surface_region)
{
  Surface const surface(TextureId{0, 1}, geom::frect(0.5f, 0.0f, 1.0f, 0.5f), geom::fsize(32, 32));
  Surface const part = surface.region(geom::frect(16, 0, 32, 16));
  EXPECT_EQ(part.get_texture(), surface.get_texture());
  EXPECT_EQ(part.get_size(), geom::fsize(16, 16));
  EXPECT_EQ(part.get_uv(), geom::frect(0.75f, 0.0f, 1.0f, 0.25f));
}

TEST(CanvasTest, fill_screen_uses_clip_space)
{
  Canvas canvas;
  canvas.translate(100, 100);
  canvas.fill_screen(surf::Color(1, 1, 1));
  ASSERT_EQ(canvas.get_commands().size(), 1u);
  EXPECT_EQ(canvas.get_commands()[0].space, Space::Clip);
  EXPECT_FLOAT_EQ(canvas.get_vertices()[0].x, -1.0f);
  EXPECT_EQ(canvas.get_space(), Space::World);
}

TEST(CanvasTest, clear_resets_state_but_keeps_layers)
{
  Canvas canvas;
  canvas.set_layer_unordered(5.0f);
  canvas.save();
  canvas.translate(5, 5);
  canvas.set_z(5.0f);
  canvas.clip(geom::frect(0, 0, 1, 1));
  canvas.fill_rect(geom::frect(0, 0, 1, 1), surf::Color(1, 1, 1));
  canvas.clear();

  EXPECT_TRUE(canvas.empty());
  EXPECT_EQ(canvas.get_transform(), glm::mat3(1.0f));
  EXPECT_EQ(canvas.get_z(), 0.0f);
  EXPECT_EQ(canvas.get_clip_rects().size(), 1u);
  EXPECT_THROW(canvas.restore(), std::logic_error);
  EXPECT_TRUE(canvas.is_layer_unordered(5.0f));
  EXPECT_FALSE(canvas.is_layer_unordered(0.0f));
}

TEST(CanvasTest, packed_color_byte_order)
{
  PackedColor const c = pack_color(surf::Color::from_rgba8888(1, 2, 3, 4));
  auto const* bytes = reinterpret_cast<uint8_t const*>(&c);
  EXPECT_EQ(bytes[0], 1);
  EXPECT_EQ(bytes[1], 2);
  EXPECT_EQ(bytes[2], 3);
  EXPECT_EQ(bytes[3], 4);
  EXPECT_EQ(unpack_color(c), surf::Color::from_rgba8888(1, 2, 3, 4));
}

/* EOF */
