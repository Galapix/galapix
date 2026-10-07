#include <gtest/gtest.h>

#include <png.h>

#include <geom/size.hpp>
#include <geom/io.hpp>

#include <surf/software_surface.hpp>
#include "plugins/png.hpp"

using namespace surf;

TEST(PNGTest, get_size__mem)
{
}

TEST(PNGTest, get_size__file)
{
  geom::isize result;
  EXPECT_TRUE(png::get_size("test/data/rgb.png", result));
  EXPECT_EQ(result, geom::isize(128, 64));
}

TEST(PNGTest, is_png)
{
  EXPECT_TRUE(png::is_png("test/data/rgb.png"));
  EXPECT_TRUE(png::is_png("test/data/rgba.png"));
  EXPECT_FALSE(png::is_png("test/data/rgb.jpg"));
}

TEST(PNGTest, load_from_file)
{
  png::load_from_file("test/data/rgb.png");
  png::load_from_file("test/data/rgba.png");
}

TEST(PNGTest, load_from_mem)
{
}

TEST(PNGTest, save__file)
{
  SoftwareSurface input_surface = png::load_from_file("test/data/rgb-8x4.png");
  std::filesystem::path outfile = std::filesystem::path(testing::TempDir()) / "PNGTest__save__file.png";
  png::save(input_surface, outfile);

  EXPECT_EQ(input_surface, png::load_from_file(outfile));
}

TEST(PNGTest, save__mem)
{
}

namespace {

/** Read the first red sample of a 16bit PNG with plain libpng,
    bypassing surf, to verify the byte order on disk */
uint16_t read_first_red16(std::vector<uint8_t> const& data)
{
  png_image image{};
  image.version = PNG_IMAGE_VERSION;
  if (!png_image_begin_read_from_memory(&image, data.data(), data.size())) {
    throw std::runtime_error(image.message);
  }
  image.format = PNG_FORMAT_LINEAR_RGB;
  std::vector<uint16_t> buffer(PNG_IMAGE_SIZE(image) / 2);
  if (!png_image_finish_read(&image, nullptr, buffer.data(), 0, nullptr)) {
    throw std::runtime_error(image.message);
  }
  return buffer[0];
}

} // namespace

TEST(PNGTest, rgb16_byte_order)
{
  // the linear 16bit format of the simplified libpng API doesn't
  // apply gamma when the file has no gamma information, so values
  // come through unchanged
  PixelData<RGB16Pixel> img({1, 1}, RGB16Pixel{0x1234, 0x5678, 0x9abc});
  std::vector<uint8_t> const data = png::save(SoftwareSurface(img));

  EXPECT_EQ(read_first_red16(data), 0x1234);

  SoftwareSurface const loaded = png::load_from_mem(data);
  ASSERT_EQ(loaded.get_format(), PixelFormat::RGB16);
  EXPECT_EQ(loaded.as_pixelview<RGB16Pixel>().get_pixel({0, 0}), (RGB16Pixel{0x1234, 0x5678, 0x9abc}));
}

TEST(PNGTest, load_from_mem__truncated)
{
  std::vector<uint8_t> data = png::save(SoftwareSurface::create(PixelFormat::RGB8, {16, 16}));
  data.resize(data.size() / 2);
  EXPECT_THROW(png::load_from_mem(data), std::runtime_error);
}

/* EOF */
