#include <gtest/gtest.h>

#include <surf/channel.hpp>
#include <surf/software_surface.hpp>

using namespace surf;

TEST(ChannelTest, split_join_rgba)
{
  PixelData<RGBA8Pixel> src({2, 2}, RGBA8Pixel{1, 2, 3, 4});

  std::vector<PixelData<L8Pixel>> channels = split_channel(src);
  ASSERT_EQ(channels.size(), 4u);
  EXPECT_EQ(channels[0].get_pixel({0, 0}).l, 1);
  EXPECT_EQ(channels[1].get_pixel({0, 0}).l, 2);
  EXPECT_EQ(channels[2].get_pixel({0, 0}).l, 3);
  EXPECT_EQ(channels[3].get_pixel({0, 0}).l, 4);

  EXPECT_EQ(join_channel(channels[0], channels[1], channels[2], channels[3]), src);
}

TEST(ChannelTest, split_join_surface)
{
  SoftwareSurface const src(PixelData<RGBA8Pixel>({2, 2}, RGBA8Pixel{1, 2, 3, 4}));
  std::vector<SoftwareSurface> channels = split_channel(src);
  ASSERT_EQ(channels.size(), 4u);
  EXPECT_EQ(join_channel(channels), src);
}

/* EOF */
