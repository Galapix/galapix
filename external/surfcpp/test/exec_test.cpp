#ifdef HAVE_EXEC

#include <gtest/gtest.h>

#include "util/exec.hpp"

using namespace surf;

TEST(ExecTest, large_stdin)
{
  // larger than a pipe buffer, would deadlock if stdin was written
  // completely before reading stdout
  std::vector<uint8_t> data(4 * 1024 * 1024);
  for (size_t i = 0; i < data.size(); ++i) {
    data[i] = static_cast<uint8_t>(i);
  }

  Exec cat("cat");
  cat.set_stdin(data);
  ASSERT_EQ(cat.exec(), 0);
  std::span<uint8_t const> const out = cat.get_stdout();
  EXPECT_TRUE(std::equal(out.begin(), out.end(), data.begin(), data.end()));
}

TEST(ExecTest, child_ignores_stdin)
{
  Exec head("head");
  head.arg("-c").arg("4");
  head.set_stdin(std::vector<uint8_t>(4 * 1024 * 1024, 'x'));
  EXPECT_EQ(head.exec(), 0);
  EXPECT_EQ(head.get_stdout_txt(), "xxxx");
}

TEST(ExecTest, exit_status)
{
  EXPECT_EQ(Exec("true").exec(), 0);
  EXPECT_EQ(Exec("false").exec(), 1);
}

TEST(ExecTest, killed_by_signal)
{
  Exec sh("sh");
  sh.arg("-c").arg("kill -9 $$");
  EXPECT_THROW(sh.exec(), std::runtime_error);
}

#endif

/* EOF */
