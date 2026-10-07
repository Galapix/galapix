#include <gtest/gtest.h>

#include <string_view>

#include "plugins/pnm.hpp"

using namespace surf;

namespace {

std::span<uint8_t const> as_bytes(std::string_view text)
{
  return {reinterpret_cast<uint8_t const*>(text.data()), text.size()};
}

} // namespace

TEST(PNMTest, load_from_mem)
{
  std::string_view const data("P6\n2 1\n255\n\x01\x02\x03\x04\x05\x06", 17);
  SoftwareSurface const surface = pnm::load_from_mem(as_bytes(data));
  ASSERT_EQ(surface.get_size(), geom::isize(2, 1));
  EXPECT_EQ(surface.as_pixelview<RGB8Pixel>().get_pixel({1, 0}), (RGB8Pixel{4, 5, 6}));
}

TEST(PNMTest, load_from_mem__invalid)
{
  EXPECT_THROW(pnm::load_from_mem(as_bytes("P6\n2 1\n")), std::runtime_error);
  EXPECT_THROW(pnm::load_from_mem(as_bytes("P6\n2 1\n65535\n")), std::runtime_error);
  EXPECT_THROW(pnm::load_from_mem(as_bytes("P6\n-2 1\n255\n")), std::runtime_error);
  EXPECT_THROW(pnm::load_from_mem(as_bytes("P6\n2 1\n255\n\x01")), std::runtime_error);
}

/* EOF */
