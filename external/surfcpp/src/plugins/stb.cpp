// surf - Software surface library
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "plugins/stb.hpp"

#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "pixel.hpp"
#include "pixel_data.hpp"
#include "software_surface.hpp"
#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"

// Static, so the stb symbols don't clash with other copies of stb in
// the same program
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#if defined(__GNUC__) && !defined(__clang__)
#  pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
#include <stb_image.h>
#include <stb_image_write.h>
#pragma GCC diagnostic pop

namespace surf {
namespace stb {

namespace {

template<typename Pixel>
SoftwareSurface make_surface(geom::isize const& size, uint8_t const* data)
{
  PixelData<Pixel> dst(size);
  std::memcpy(dst.get_data(), data, static_cast<size_t>(geom::area(size)) * sizeof(Pixel));
  return SoftwareSurface(std::move(dst));
}

/** The 8 bit format with the channels of `format` and the number of
    channels, which is what stb_image_write takes */
std::pair<PixelFormat, int> stb_format(PixelFormat format)
{
  switch (format)
  {
    case PixelFormat::L8:
    case PixelFormat::L16:
    case PixelFormat::L32:
    case PixelFormat::L32f:
    case PixelFormat::L64f:
      return {PixelFormat::L8, 1};

    case PixelFormat::LA8:
    case PixelFormat::LA16:
    case PixelFormat::LA32:
    case PixelFormat::LA32f:
    case PixelFormat::LA64f:
      return {PixelFormat::LA8, 2};

    case PixelFormat::RGB8:
    case PixelFormat::RGB16:
    case PixelFormat::RGB32:
    case PixelFormat::RGB32f:
    case PixelFormat::RGB64f:
      return {PixelFormat::RGB8, 3};

    case PixelFormat::RGBA8:
    case PixelFormat::RGBA16:
    case PixelFormat::RGBA32:
    case PixelFormat::RGBA32f:
    case PixelFormat::RGBA64f:
      return {PixelFormat::RGBA8, 4};

    case PixelFormat::NONE:
    default:
      throw std::invalid_argument("stb: can't save surface without a pixel format");
  }
}

/** The surface as tightly packed 8 bit rows */
std::vector<uint8_t> packed_pixels(SoftwareSurface const& surface, PixelFormat format, int comp)
{
  SoftwareSurface const converted = (surface.get_format() == format) ? surface : convert(surface, format);

  size_t const row_size = static_cast<size_t>(converted.get_width()) * static_cast<size_t>(comp);
  std::vector<uint8_t> pixels(row_size * static_cast<size_t>(converted.get_height()));
  for (int y = 0; y < converted.get_height(); ++y) {
    std::memcpy(pixels.data() + row_size * static_cast<size_t>(y), converted.get_row_data(y), row_size);
  }
  return pixels;
}

void append_to_vector(void* context, void* data, int size)
{
  auto& out = *static_cast<std::vector<uint8_t>*>(context);
  out.insert(out.end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + size);
}

void write_file(std::filesystem::path const& filename, std::vector<uint8_t> const& data)
{
  std::ofstream out(filename, std::ios::binary);
  out.write(reinterpret_cast<char const*>(data.data()), static_cast<std::streamsize>(data.size()));
  out.close();
  if (!out) {
    throw std::runtime_error("stb: failed to write " + filename.string());
  }
}

} // namespace

SoftwareSurface load_from_mem(std::span<uint8_t const> data)
{
  int width = 0;
  int height = 0;
  int comp = 0;
  std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
    stbi_load_from_memory(data.data(), static_cast<int>(data.size()), &width, &height, &comp, 0),
    &stbi_image_free);
  if (!pixels) {
    throw std::runtime_error(std::string("stb: failed to load image: ") + stbi_failure_reason());
  }

  geom::isize const size(width, height);
  switch (comp)
  {
    case 1: return make_surface<L8Pixel>(size, pixels.get());
    case 2: return make_surface<LA8Pixel>(size, pixels.get());
    case 3: return make_surface<RGB8Pixel>(size, pixels.get());
    case 4: return make_surface<RGBA8Pixel>(size, pixels.get());
    default: throw std::runtime_error("stb: unsupported number of channels: " + std::to_string(comp));
  }
}

SoftwareSurface load_from_file(std::filesystem::path const& filename)
{
  std::ifstream in(filename, std::ios::binary);
  if (!in) {
    throw std::runtime_error("stb: couldn't open " + filename.string());
  }
  std::vector<uint8_t> const data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

  try {
    return load_from_mem(data);
  } catch (std::exception const& err) {
    throw std::runtime_error(filename.string() + ": " + err.what());
  }
}

std::vector<uint8_t> save_png(SoftwareSurface const& surface)
{
  auto const [format, comp] = stb_format(surface.get_format());
  std::vector<uint8_t> const pixels = packed_pixels(surface, format, comp);

  std::vector<uint8_t> out;
  if (!stbi_write_png_to_func(&append_to_vector, &out, surface.get_width(), surface.get_height(), comp,
                              pixels.data(), surface.get_width() * comp)) {
    throw std::runtime_error("stb: failed to encode PNG");
  }
  return out;
}

void save_png(SoftwareSurface const& surface, std::filesystem::path const& filename)
{
  write_file(filename, save_png(surface));
}

std::vector<uint8_t> save_jpeg(SoftwareSurface const& surface, int quality)
{
  auto const [format, comp] = stb_format(surface.get_format());
  std::vector<uint8_t> const pixels = packed_pixels(surface, format, comp);

  std::vector<uint8_t> out;
  if (!stbi_write_jpg_to_func(&append_to_vector, &out, surface.get_width(), surface.get_height(), comp,
                              pixels.data(), quality)) {
    throw std::runtime_error("stb: failed to encode JPEG");
  }
  return out;
}

void save_jpeg(SoftwareSurface const& surface, std::filesystem::path const& filename, int quality)
{
  write_file(filename, save_jpeg(surface, quality));
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("stb", load_from_file, load_from_mem);

  factory.register_by_magic(*loader, "\x89PNG");
  factory.register_by_magic(*loader, "\xff\xd8");

  factory.register_by_mime_type(*loader, "image/png");
  factory.register_by_mime_type(*loader, "image/x-png");
  factory.register_by_mime_type(*loader, "image/jpeg");

  factory.register_by_extension(*loader, "png");
  factory.register_by_extension(*loader, "jpeg");
  factory.register_by_extension(*loader, "jpg");

  factory.add_loader(std::move(loader));
}

} // namespace stb
} // namespace surf

/* EOF */
