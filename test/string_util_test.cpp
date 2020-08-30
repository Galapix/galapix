#include <gtest/gtest.h>

#include "util/string_util.hpp"

TEST(StringUtilTest, has_prefix)
{
  EXPECT_FALSE(StringUtil::has_prefix("HelloWorld", "World"));
  EXPECT_TRUE(StringUtil::has_prefix("HelloWorld", "Hello"));
  EXPECT_FALSE(StringUtil::has_prefix("Very", "VeryLongPrefix"));
  EXPECT_FALSE(StringUtil::has_prefix("Prefix", "VeryLongPrefix"));
  EXPECT_TRUE(StringUtil::has_prefix("EmptyString", ""));
  EXPECT_FALSE(StringUtil::has_prefix("", "EmptyString"));
  EXPECT_TRUE(StringUtil::has_prefix("", ""));
}

TEST(StringUtilTest, has_suffix)
{
  EXPECT_TRUE(StringUtil::has_suffix("HelloWorld", "World"));
  EXPECT_FALSE(StringUtil::has_suffix("HelloWorld", "Hello"));
  EXPECT_FALSE(StringUtil::has_prefix("Suffix", "VeryLongSuffix"));
  EXPECT_FALSE(StringUtil::has_prefix("Very", "VeryLongSuffix"));
  EXPECT_TRUE(StringUtil::has_suffix("EmptyString", ""));
  EXPECT_FALSE(StringUtil::has_suffix("", "EmptyString"));
  EXPECT_TRUE(StringUtil::has_suffix("", ""));
}

TEST(StringUtilTest, numeric_less)
{
  EXPECT_TRUE(StringUtil::numeric_less("1", "2"));
  EXPECT_TRUE(StringUtil::numeric_less("001", "002"));
  EXPECT_TRUE(StringUtil::numeric_less("999", "1111"));
  EXPECT_TRUE(StringUtil::numeric_less("01", "002"));
  EXPECT_TRUE(StringUtil::numeric_less("01", "02"));

  // FIXME: StringUtil::numeric_less() doesn't deal with leading zero, i.e. "01" > "2"
  // EXPECT_TRUE(StringUtil::numeric_less("01", "2"));
  // EXPECT_TRUE(StringUtil::numeric_less("text01", "text2"));

  EXPECT_TRUE(StringUtil::numeric_less("text10-5", "text10-10"));

  EXPECT_FALSE(StringUtil::numeric_less("1111", "999"));
}

TEST(StringUtilTest, string_tokenize)
{
  EXPECT_EQ(string_tokenize("a b c d", ' '), (std::vector<std::string>{"a", "b", "c", "d"}));
  EXPECT_EQ(string_tokenize("  a1   b2  c3  d4  ", ' '), (std::vector<std::string>{"a1", "b2", "c3", "d4"}));
  EXPECT_EQ(string_tokenize("xxa1xxxb2xxc3xxd4xxx", 'x'), (std::vector<std::string>{"a1", "b2", "c3", "d4"}));
}

/* EOF */
