#ifndef HEADER_SURF_ALGORITHM_HPP
#define HEADER_SURF_ALGORITHM_HPP

#include <geom/rect.hpp>

#include "pixel_view.hpp"

namespace surf {

template<typename Pixel, typename UnaryFunction>
void for_each_pixel(PixelView<Pixel>& src, UnaryFunction f)
{
  int const width = src.get_width();
  int const height = src.get_height();

  for(int y = 0; y < height; ++y) {
    Pixel* row = src.get_row(y);
    for(int x = 0; x < width; ++x) {
      f(row[x]);
    }
  }
}

template<typename Pixel, typename UnaryFunction>
void for_each_pixel(PixelView<Pixel>& src, geom::irect const& rect, UnaryFunction f)
{
  for(int y = rect.top(); y < rect.bottom(); ++y) {
    Pixel* row = src.get_row(y);
    for(int x = rect.left(); x < rect.right(); ++x) {
      f(row[x]);
    }
  }
}

} // namespace surf

#endif

/* EOF */
