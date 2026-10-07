// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SURF_BLIT_HPP
#define HEADER_SURF_BLIT_HPP

#include <cassert>
#include <cstring>

#include "blend.hpp"
#include "blendfunc.hpp"
#include "pixel_data.hpp"

namespace surf {

namespace detail {

/** Generate a 'dstrect' such that it's relations are the same as
    between 'srcrect_unclipped' and 'srcrect'.. */
inline
geom::irect equivalence_clip(geom::irect const& srcrect_unclipped, geom::irect const& srcrect,
                             geom::irect const& dstrect_unclipped)
{
  return geom::irect(dstrect_unclipped.left() + (srcrect.left() - srcrect_unclipped.left()) * dstrect_unclipped.width() / srcrect_unclipped.width(),
                     dstrect_unclipped.top() + (srcrect.top() - srcrect_unclipped.top()) * dstrect_unclipped.height() / srcrect_unclipped.height(),
                     dstrect_unclipped.right() + (srcrect.right() - srcrect_unclipped.right()) * dstrect_unclipped.width() / srcrect_unclipped.width(),
                     dstrect_unclipped.bottom() + (srcrect.bottom() - srcrect_unclipped.bottom()) * dstrect_unclipped.height() / srcrect_unclipped.height());
}

template<typename SrcPixel, typename DstPixel, typename BlendFunc> inline
void blend_n(BlendFunc blend_func,
             SrcPixel const* srcpixels, DstPixel* dstpixels,
             size_t count)
{
  for (size_t i = 0; i < count; ++i) {
    dstpixels[i] = blend_func(srcpixels[i], dstpixels[i]);
  }
}

} // namespace detail

namespace experimental {

template<typename SrcPixel, typename DstPixel>
void blit__slow(PixelView<SrcPixel>& src, PixelView<DstPixel>& dst, const geom::ipoint& pos)
{
  int const start_x = std::max(0, -pos.x());
  int const start_y = std::max(0, -pos.y());

  int const end_x = std::min(src.get_size().width(), dst.get_size().width()  - pos.x());
  int const end_y = std::min(src.get_size().height(), dst.get_size().height() - pos.y());

  for(int y = start_y; y < end_y; ++y) {
    for(int x = start_x; x < end_x; ++x) {
      SrcPixel const srcpx = src.get_pixel(geom::ipoint(x, y));
      dst.put_pixel(geom::ipoint(pos.x() + x, pos.y() + y), convert<SrcPixel, DstPixel>(srcpx));
    }
  }
}

template<typename SrcPixel, typename DstPixel,
         std::enable_if_t<std::is_same<SrcPixel, DstPixel>::value, int> = 0>
void blit__copy(PixelView<SrcPixel> const& src, geom::irect const& srcrect,
                PixelView<DstPixel>& dst, const geom::ipoint& pos)
{
  assert(contains(geom::irect(src.get_size()), srcrect));

  geom::irect const cliprect(dst.get_size());
  geom::irect const region = intersection(geom::irect(srcrect.size()) + geom::ioffset(pos), cliprect);
  if (region.size().is_empty()) {
    return;
  }
  geom::ioffset const dst2src(-pos.x() + srcrect.left(), -pos.y() + srcrect.top());

  for (int y = region.top(); y < region.bottom(); ++y) {
    std::copy_n(src.get_row(y + dst2src.y()) + region.left() + dst2src.x(),
                region.width(),
                dst.get_row(y) + region.left());
  }
}

} // namespace experimental

template<typename SrcPixel, typename DstPixel,
         std::enable_if_t<std::is_same<SrcPixel, DstPixel>::value, int> = 0>
void blit(PixelView<SrcPixel> const& src, geom::irect const& srcrect,
          PixelView<DstPixel>& dst, const geom::ipoint& pos)
{
  assert(contains(geom::irect(src.get_size()), srcrect));

  geom::irect const cliprect(dst.get_size());
  geom::irect const region = intersection(geom::irect(srcrect.size()) + geom::ioffset(pos), cliprect);
  if (region.size().is_empty()) {
    return;
  }
  geom::ioffset const dst2src(-pos.x() + srcrect.left(), -pos.y() + srcrect.top());

  for (int y = region.top(); y < region.bottom(); ++y) {
    std::memcpy(dst.get_row(y) + region.left(),
                src.get_row(y + dst2src.y()) + region.left() + dst2src.x(),
                region.width() * sizeof(SrcPixel));
  }
}

template<typename SrcPixel, typename DstPixel,
         std::enable_if_t<!std::is_same<SrcPixel, DstPixel>::value, int> = 0>
void blit(PixelView<SrcPixel> const& src, geom::irect const& srcrect,
          PixelView<DstPixel>& dst, const geom::ipoint& pos)
{
  assert(contains(geom::irect(src.get_size()), srcrect));

  geom::irect const cliprect(dst.get_size());
  geom::irect const region = intersection(geom::irect(srcrect.size()) + geom::ioffset(pos), cliprect);
  if (region.size().is_empty()) {
    return;
  }
  geom::ioffset const dst2src(-pos.x() + srcrect.left(), -pos.y() + srcrect.top());

  for (int y = region.top(); y < region.bottom(); ++y) {
    std::transform(src.get_row(y + dst2src.y()) + region.left() + dst2src.x(),
                   src.get_row(y + dst2src.y()) + region.left() + dst2src.x() + region.width(),
                   dst.get_row(y) + region.left(),
                   convert<SrcPixel, DstPixel>);
  }
}

/** Performs a simple copy from this to \a test, no blending is performed */
template<typename SrcPixel, typename DstPixel>
void blit(PixelView<SrcPixel> const& src, PixelView<DstPixel>& dst, geom::ipoint const& pos)
{
  blit(src, geom::irect(src.get_size()), dst, pos);
}

template<typename BlendFuncType, typename SrcPixel, typename DstPixel>
void blend_scaled(BlendFuncType blendfunc,
                  PixelView<SrcPixel> const& src, geom::irect const& srcrect_unclipped,
                  PixelView<DstPixel>& dst, geom::irect const& dstrect_unclipped)
{
  if (srcrect_unclipped.size().is_empty() || dstrect_unclipped.size().is_empty()) {
    return;
  }

  geom::irect const dstrect1 = geom::intersection(geom::irect(dst.get_size()), dstrect_unclipped);
  if (dstrect1.size().is_empty()) {
    return;
  }
  geom::irect const srcrect1 = detail::equivalence_clip(dstrect_unclipped, dstrect1, srcrect_unclipped);

  geom::irect const srcrect = geom::intersection(geom::irect(src.get_size()), srcrect1);
  if (srcrect1.size().is_empty() || srcrect.size().is_empty()) {
    return;
  }
  geom::irect const dstrect = detail::equivalence_clip(srcrect1, srcrect, dstrect1);
  if (dstrect.size().is_empty()) {
    return;
  }

  assert(contains(geom::irect(src.get_size()), srcrect));
  assert(contains(geom::irect(dst.get_size()), dstrect));

  for (int y = 0; y < dstrect.height(); ++y) {
    SrcPixel const* const srcrow = src.get_row(srcrect.top() + y * srcrect.height() / dstrect.height()) + srcrect.left();
    DstPixel* const dstrow = dst.get_row(y + dstrect.top()) + dstrect.left();

    for (int x = 0; x < dstrect.width(); ++x) {
      // LUT this?
      dstrow[x] = blendfunc(srcrow[x * srcrect.width() / dstrect.width()], dstrow[x]);
    }
  }
}

template<typename BlendFunc, typename SrcPixel, typename DstPixel>
void blend_scaled(BlendFunc blendfunc,
                  PixelView<SrcPixel> const& src,
                  PixelView<DstPixel>& dst, geom::irect const& dstrect)
{
  blend_scaled(blendfunc, src, geom::irect(src.get_size()), dst, dstrect);
}

template<typename SrcPixel, typename DstPixel>
void blend_scaled_wrap(BlendFunc blendfunc,
                       PixelView<SrcPixel> const& src, geom::irect const& srcrect_unclipped,
                       PixelView<DstPixel>& dst, geom::irect const& dstrect_unclipped)
{
  using srctype = SrcPixel;
  using dsttype = DstPixel;

  BLENDFUNC_TO_TYPE(
    blendfunc,
    blendfunc_type,
    blend_scaled(blendfunc_type(),
                 src, srcrect_unclipped,
                 dst, dstrect_unclipped));
}

template<typename BlendFunc, typename SrcPixel, typename DstPixel> inline
void blend(BlendFunc blend_func,
           PixelView<SrcPixel> const& src, geom::irect const& srcrect,
           PixelView<DstPixel>& dst, const geom::ipoint& pos)
{
  assert(contains(geom::irect(src.get_size()), srcrect));

  geom::irect const cliprect(dst.get_size());
  geom::irect const region = intersection(geom::irect(srcrect.size()) + geom::ioffset(pos), cliprect);
  if (region.size().is_empty()) {
    return;
  }
  geom::ioffset const dst2src(-pos.x() + srcrect.left(), -pos.y() + srcrect.top());

  for (int y = region.top(); y < region.bottom(); ++y) {
    detail::blend_n(
      blend_func,
      src.get_row(y + dst2src.y()) + region.left() + dst2src.x(),
      dst.get_row(y) + region.left(),
      region.width());
  }
}

template<typename BlendFunc, typename SrcPixel, typename DstPixel>
void blend(BlendFunc blend_func,
           PixelView<SrcPixel> const& src, PixelView<DstPixel>& dst, geom::ipoint const& pos)
{
  blend(blend_func, src, geom::irect(src.get_size()), dst, pos);
}

template<typename SrcPixel, typename DstPixel>
void blend_wrap(BlendFunc blendfunc,
                PixelView<SrcPixel> const& src, geom::irect const& srcrect_unclipped,
                PixelView<DstPixel>& dst, geom::ipoint const& pos)
{
  using srctype = SrcPixel;
  using dsttype = DstPixel;

  BLENDFUNC_TO_TYPE(
    blendfunc,
    blendfunc_type,
    blend(blendfunc_type(),
          src, srcrect_unclipped,
          dst, pos));
}

} // namespace surf

#endif

/* EOF */
