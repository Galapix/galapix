#include <string.h>
#include <algorithm>
#include <vector>
#include <benchmark/benchmark.h>

#include <surf/pixel_data.hpp>
#include <surf/transform.hpp>

using namespace surf;

namespace {

const geom::isize DSTSIZE(1024, 1024);

void BM_transform(::benchmark::State& state, Transform mod)
{
  PixelData<RGBAPixel> src(DSTSIZE, RGBAPixel{255, 255, 255, 255});

  while (state.KeepRunning()) {
    PixelData<RGBAPixel> dst = transform(src, mod);
    benchmark::DoNotOptimize(dst);
  }
}

} // namespace

BENCHMARK_CAPTURE(BM_transform, rotate0, Transform::ROTATE_0);
BENCHMARK_CAPTURE(BM_transform, rotate90, Transform::ROTATE_90);
BENCHMARK_CAPTURE(BM_transform, rotate180, Transform::ROTATE_180);
BENCHMARK_CAPTURE(BM_transform, rotate270, Transform::ROTATE_270);

BENCHMARK_CAPTURE(BM_transform, rotate0flip, Transform::ROTATE_0_FLIP);
BENCHMARK_CAPTURE(BM_transform, rotate90flip, Transform::ROTATE_90_FLIP);
BENCHMARK_CAPTURE(BM_transform, rotate180flip, Transform::ROTATE_180_FLIP);
BENCHMARK_CAPTURE(BM_transform, rotate270flip, Transform::ROTATE_270_FLIP);

/* EOF */
