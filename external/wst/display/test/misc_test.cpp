// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <wstdisplay/blend.hpp>
#include <wstdisplay/font/font.hpp>
#include <wstdisplay/view.hpp>

using namespace wstdisplay;

TEST(Utf8Test, decode)
{
  std::string_view const text = "a\xc3\xbc\xe2\x82\xac\xf0\x9f\x98\x80";
  size_t pos = 0;
  EXPECT_EQ(utf8_next(text, pos), U'a');
  EXPECT_EQ(utf8_next(text, pos), U'ü');
  EXPECT_EQ(utf8_next(text, pos), U'€');
  EXPECT_EQ(utf8_next(text, pos), U'\U0001f600');
  EXPECT_EQ(pos, text.size());
}

TEST(Utf8Test, invalid)
{
  std::string_view const text = "\xff\xc3";
  size_t pos = 0;
  EXPECT_EQ(utf8_next(text, pos), U'�');
  EXPECT_EQ(utf8_next(text, pos), U'�');
  EXPECT_EQ(pos, text.size());
}

TEST(ViewTest, default_view_is_identity)
{
  View view(geom::isize(800, 600));
  EXPECT_EQ(view.world_to_screen(geom::fpoint(10, 20)), geom::fpoint(10, 20));
  EXPECT_EQ(view.get_clip_rect(), geom::frect(0, 0, 800, 600));
}

TEST(ViewTest, zoom_around_point)
{
  View view(geom::isize(800, 600));
  view.set_pos(geom::fpoint(1000, 1000));
  geom::fpoint const screen(100, 200);
  geom::fpoint const world = view.screen_to_world(screen);
  view.set_zoom(screen, 3.0f);
  geom::fpoint const after = view.screen_to_world(screen);
  EXPECT_NEAR(after.x(), world.x(), 1e-3f);
  EXPECT_NEAR(after.y(), world.y(), 1e-3f);
}

TEST(ViewTest, roundtrip_with_rotation)
{
  View view(geom::isize(800, 600));
  view.set_pos(geom::fpoint(-50, 70));
  view.set_zoom(0.5f);
  view.set_rotation(33.0f);
  geom::fpoint const p(123, 456);
  geom::fpoint const q = view.screen_to_world(view.world_to_screen(p));
  EXPECT_NEAR(q.x(), p.x(), 1e-3f);
  EXPECT_NEAR(q.y(), p.y(), 1e-3f);
}

TEST(ViewTest, zoom_to)
{
  View view(geom::isize(800, 600));
  view.zoom_to(geom::frect(0, 0, 400, 100));
  EXPECT_FLOAT_EQ(view.get_zoom(), 2.0f);
  EXPECT_EQ(view.get_pos(), geom::fpoint(200, 50));
}

TEST(BlendTest, roundtrip)
{
  for (Blend blend : {Blend::Opaque, Blend::Alpha, Blend::Premultiplied, Blend::Add, Blend::Multiply}) {
    EXPECT_EQ(blend_from_string(to_string(blend)), blend);
  }
  EXPECT_THROW(blend_from_string("foo"), std::invalid_argument);
}

/* EOF */
