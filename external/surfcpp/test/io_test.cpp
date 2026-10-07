#include <gtest/gtest.h>

#include <sstream>

#include <surf/io.hpp>

using namespace surf;

TEST(IOTest, rgb32)
{
  std::ostringstream os;
  os << RGB32Pixel{0xf0000000u, 0, 0xffffffffu};
  EXPECT_EQ(os.str(), "(f0000000 00000000 ffffffff)");
}

/* EOF */
