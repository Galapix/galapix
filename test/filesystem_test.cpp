#include <gtest/gtest.h>

#include "util/filesystem.hpp"

using namespace surf;

TEST(FilesystemTest, get_extension)
{
  EXPECT_EQ(Filesystem::get_extension("foo.png"), "png");
  EXPECT_EQ(Filesystem::get_extension("/tmp/foo.PNG"), "png");
  EXPECT_EQ(Filesystem::get_extension("foo.xcf.gz"), "xcf.gz");
  EXPECT_EQ(Filesystem::get_extension("foo"), "");
  EXPECT_EQ(Filesystem::get_extension("/tmp/dir.d/foo"), "");
  EXPECT_EQ(Filesystem::get_extension("foo.gz"), "gz");
}

/* EOF */
