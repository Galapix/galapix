#include <gtest/gtest.h>

#include <surf/hsv.hpp>

using namespace surf;

TEST(HSVTest, hsv_from_color)
{
  HSVColor hsv;

  hsv = hsv_from_color(Color(1.0f, 0.0f, 0.0f));
  EXPECT_FLOAT_EQ(hsv.hue, 0.0f);
  EXPECT_FLOAT_EQ(hsv.value, 1.0f);
  EXPECT_FLOAT_EQ(hsv.saturation, 1.0f);

  hsv = hsv_from_color(Color(0.5f, 0.0f, 0.0f));
  EXPECT_FLOAT_EQ(hsv.hue, 0.0f);
  EXPECT_FLOAT_EQ(hsv.value, 0.5f);
  EXPECT_FLOAT_EQ(hsv.saturation, 1.0f);

  hsv = hsv_from_color(Color(1.0f, 0.5f, 0.5f));
  EXPECT_FLOAT_EQ(hsv.hue, 0.0f);
  EXPECT_FLOAT_EQ(hsv.value, 1.0f);
  EXPECT_FLOAT_EQ(hsv.saturation, 0.5f);

  hsv = hsv_from_color(Color(0.0f, 1.0f, 0.0f));
  EXPECT_FLOAT_EQ(hsv.hue, 0.33333334f);
  EXPECT_FLOAT_EQ(hsv.value, 1.0f);
  EXPECT_FLOAT_EQ(hsv.saturation, 1.0f);
}

TEST(HSVTest, hsv_from_color__hue_range)
{
  // red with some blue gives a hue just below 1.0, not a negative one
  HSVColor const hsv = hsv_from_color(Color(1.0f, 0.0f, 0.5f));
  EXPECT_GE(hsv.hue, 0.0f);
  EXPECT_LT(hsv.hue, 1.0f);
  EXPECT_FLOAT_EQ(hsv.hue, 11.0f / 12.0f);
}

TEST(HSVTest, color_from_hue__wrap)
{
  Color const red = color_from_hue(0.0f);
  EXPECT_EQ(color_from_hue(1.0f), red);
  EXPECT_EQ(color_from_hue(-1.0f), red);

  Color const almost_red = color_from_hue(0.9999999f);
  EXPECT_NEAR(almost_red.r, 1.0f, 0.001f);
  EXPECT_NEAR(almost_red.g, 0.0f, 0.001f);
  EXPECT_NEAR(almost_red.b, 0.0f, 0.001f);
}

TEST(HSVTest, roundtrip)
{
  Color const color(1.0f, 0.0f, 0.5f);
  Color const result = color_from_hsv(hsv_from_color(color));
  EXPECT_NEAR(result.r, color.r, 0.0001f);
  EXPECT_NEAR(result.g, color.g, 0.0001f);
  EXPECT_NEAR(result.b, color.b, 0.0001f);
}

TEST(HSVTest, color_from_hue)
{
  Color color;

  color = color_from_hue(0.0f);
  EXPECT_FLOAT_EQ(color.r, 1.0f);
  EXPECT_FLOAT_EQ(color.g, 0.0f);
  EXPECT_FLOAT_EQ(color.b, 0.0f);
}


TEST(HSVTest, color_from_hsv)
{
  Color color;

  color = color_from_hsv(HSVColor{0.0f, 1.0f, 1.0f});
  EXPECT_FLOAT_EQ(color.r, 1.0f);
  EXPECT_FLOAT_EQ(color.g, 0.0f);
  EXPECT_FLOAT_EQ(color.b, 0.0f);

  color = color_from_hsv(HSVColor{0.0f, 1.0f, 0.5f});
  EXPECT_FLOAT_EQ(color.r, 0.5f);
  EXPECT_FLOAT_EQ(color.g, 0.0f);
  EXPECT_FLOAT_EQ(color.b, 0.0f);
}

/* EOF */
