#include <gtest/gtest.h>

#include "util/sha1.hpp"

using namespace galapix;

TEST(SHA1Test, from_string)
{
  ASSERT_EQ(SHA1::from_string("0a4d55a8d778e5022fab701977c5d840bbc486d0").str(),
            "0a4d55a8d778e5022fab701977c5d840bbc486d0");

  ASSERT_THROW(SHA1::from_string("0a4d55a8d778e5022fab"), std::exception);
}

TEST(SHA1Test, from_mem)
{
  ASSERT_EQ(SHA1::from_mem("Hello World").str(),
            "0a4d55a8d778e5022fab701977c5d840bbc486d0");
}

TEST(SHA1Test, from_file)
{
  ASSERT_EQ(SHA1::from_file("test/data/sha1_test.txt").str(),
            "648a6a6ffffdaa0badb23b8baf90b6168dd16b3a");
  ASSERT_THROW(SHA1::from_file("/tmp/a_file_that_does_not_exist"), std::exception);
}

TEST(SHA1Test, operator_bool)
{
  ASSERT_FALSE(static_cast<bool>(SHA1()));
  ASSERT_TRUE(static_cast<bool>(SHA1::from_mem("Hello World")));
}

/* EOF */
