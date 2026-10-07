#include <string.h>
#include <algorithm>
#include <vector>
#include <benchmark/benchmark.h>

#include <surf/pixel_data.hpp>
#include <surf/filter.hpp>

using namespace surf;

namespace {

const geom::isize DSTSIZE(1024, 1024);

void BM_filter_add(::benchmark::State& state)
{
  PixelData<RGBAPixel> dst(DSTSIZE, RGBAPixel{255, 255, 255, 255});

  while (state.KeepRunning()) {
    surf::apply_add(dst, 0.25f);
  }
}

void BM_filter_brightness(::benchmark::State& state)
{
  PixelData<RGBAPixel> dst(DSTSIZE, RGBAPixel{255, 255, 255, 255});

  while (state.KeepRunning()) {
    surf::apply_brightness(dst, 0.25f);
  }
}

} // namespace

BENCHMARK(BM_filter_add);
BENCHMARK(BM_filter_brightness);

/* EOF */
