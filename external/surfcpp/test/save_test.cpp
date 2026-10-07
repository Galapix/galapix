#include <gtest/gtest.h>

#include <surf/surf.hpp>

TEST(SaveTest, save)
{
  auto img = surf::SoftwareSurface::from_file("test/data/rgb.jpg");

  ::testing::TestInfo const* const testinfo = ::testing::UnitTest::GetInstance()->current_test_info();

  std::filesystem::path outdir = testing::TempDir();

  std::ostringstream outfile;
  outfile << "surf." << testinfo->test_suite_name() << "." << testinfo->name();

  surf::save(img, outdir / (outfile.str() + ".png"), "png");
  surf::save(img, outdir / (outfile.str() + ".jpg"), "jpeg");
}

/* EOF */

