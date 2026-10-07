#include <string.h>
#include <algorithm>
#include <vector>
#include <benchmark/benchmark.h>

#include <surf/pixel_data.hpp>
#include <surf/blit.hpp>
#include <surf/fill.hpp>

using namespace surf;

namespace {

const geom::isize DSTSIZE(1024, 1024);

void BM_fill(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE, RGB8Pixel{255, 255, 255});

  while (state.KeepRunning()) {
    surf::detail::fill__slow(dst, RGB8Pixel{12, 34, 56});
  }
}

void BM_fill__filln(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE, RGB8Pixel{255, 255, 255});

  while (state.KeepRunning()) {
    detail::fill__filln(dst, RGB8Pixel{12, 34, 56});
  }
}

void BM_fill__slow(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE, RGB8Pixel{255, 255, 255});

  while (state.KeepRunning()) {
    detail::fill__slow(dst, RGB8Pixel{12, 34, 56});
  }
}

void BM_fill__slow_rgba(::benchmark::State& state)
{
  PixelData<RGBA8Pixel> dst(DSTSIZE, RGBA8Pixel{255, 255, 255, 255});

  while (state.KeepRunning()) {
    detail::fill__slow(dst, RGBA8Pixel{12, 34, 56, 123});
  }
}

void BM_fill__fast(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE, RGB8Pixel{255, 255, 255});

  while (state.KeepRunning()) {
    detail::fill__fast(dst, RGB8Pixel{12, 34, 56});
  }
}

void BM_fill__memset(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE);

  while (state.KeepRunning()) {
    detail::fill__memset(dst);
  }
}

void BM_fill_rect(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE, RGB8Pixel{255, 255, 255});

  while (state.KeepRunning()) {
    surf::fill_rect(
      dst, geom::irect(1, 1, dst.get_width() - 2, dst.get_height() - 2),
      RGB8Pixel{12, 34, 56});
  }
}

void BM_fill_rect__slow(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE, RGB8Pixel{255, 255, 255});

  while (state.KeepRunning()) {
    surf::detail::fill_rect__slow(
      dst, geom::irect(1, 1, dst.get_width() - 2, dst.get_height() - 2),
      RGB8Pixel{12, 34, 56});
  }
}

void BM_fill_rect__filln(::benchmark::State& state)
{
  PixelData<RGB8Pixel> dst(DSTSIZE, RGB8Pixel{255, 255, 255});

  while (state.KeepRunning()) {
    surf::detail::fill_rect__filln(
      dst, geom::irect(1, 1, dst.get_width() - 2, dst.get_height() - 2),
      RGB8Pixel{12, 34, 56});
  }
}

} // namespace

BENCHMARK(BM_fill);
BENCHMARK(BM_fill__slow);
BENCHMARK(BM_fill__slow_rgba);
BENCHMARK(BM_fill__fast);
BENCHMARK(BM_fill__filln);
BENCHMARK(BM_fill__memset);

BENCHMARK(BM_fill_rect);
BENCHMARK(BM_fill_rect__filln);
BENCHMARK(BM_fill_rect__slow);

/* EOF */
