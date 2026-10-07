#include <gtest/gtest.h>

#include <surf/pixel.hpp>

TEST(PixelTest, validate_sizeof)
{
  EXPECT_EQ(1, sizeof(surf::L8Pixel));
  EXPECT_EQ(3, sizeof(surf::RGB8Pixel));
  EXPECT_EQ(4, sizeof(surf::RGBA8Pixel));

  EXPECT_TRUE(std::is_trivial<surf::L8Pixel>::value);
  EXPECT_TRUE(std::is_trivial<surf::RGB8Pixel>::value);
  EXPECT_TRUE(std::is_trivial<surf::RGBA8Pixel>::value);
}

TEST(PixelTest, equality)
{
  EXPECT_EQ((surf::RGB8Pixel{1, 2, 3}), (surf::RGB8Pixel{1, 2, 3}));
  EXPECT_NE((surf::RGB8Pixel{1, 2, 3}), (surf::RGB8Pixel{3, 2, 1}));

  EXPECT_EQ((surf::RGBA8Pixel{1, 2, 3, 4}), (surf::RGBA8Pixel{1, 2, 3, 4}));
  EXPECT_NE((surf::RGBA8Pixel{1, 2, 3, 4}), (surf::RGBA8Pixel{4, 3, 2, 1}));

  EXPECT_EQ((surf::L8Pixel{1}), (surf::L8Pixel{1}));
  EXPECT_NE((surf::L8Pixel{1}), (surf::L8Pixel{4}));
}

TEST(PixelTest, max)
{
  EXPECT_EQ(surf::RGB8Pixel::max(), 255);
  EXPECT_EQ(surf::RGB16Pixel::max(), 65535);
  EXPECT_EQ(surf::RGB32Pixel::max(), 4294967295);

  EXPECT_EQ(surf::RGB32fPixel::max(), 1.0f);
  EXPECT_EQ(surf::RGB64fPixel::max(), 1.0);
}

TEST(PixelTest, alpha)
{
  EXPECT_EQ(surf::alpha(surf::RGB8Pixel{}), 255);
  EXPECT_EQ(surf::alpha(surf::RGB16Pixel{}), 65535);
  EXPECT_EQ(surf::alpha(surf::RGB32Pixel{}), 4294967295);
}

TEST(PixelTest, alpha_f)
{
  EXPECT_EQ(surf::alpha_f(surf::RGB8Pixel{}), 1.0f);
  EXPECT_EQ(surf::alpha_f(surf::RGB16Pixel{}), 1.0f);
  EXPECT_EQ(surf::alpha_f(surf::RGB32Pixel{}), 1.0f);
}

TEST(PixelTest, f2value)
{
  EXPECT_EQ(surf::f2value<surf::RGB8Pixel>(1.0f), 255);
  EXPECT_EQ(surf::f2value<surf::RGB16Pixel>(1.0f), 65535);
  EXPECT_EQ(surf::f2value<surf::RGB32Pixel>(1.0f), 4294967295);

  EXPECT_EQ(surf::f2value<surf::RGB8Pixel>(0.0f), 0);
  EXPECT_EQ(surf::f2value<surf::RGB16Pixel>(0.0f), 0);
  EXPECT_EQ(surf::f2value<surf::RGB32Pixel>(0.0f), 0);

  EXPECT_EQ(surf::f2value<surf::RGB32fPixel>(1.0f), 1.0f);
  EXPECT_EQ(surf::f2value<surf::RGB64fPixel>(1.0f), 1.0);

  EXPECT_EQ(surf::f2value<surf::RGB32fPixel>(0.0f), 0.0f);
  EXPECT_EQ(surf::f2value<surf::RGB64fPixel>(0.0f), 0.0);
}

/* EOF */
