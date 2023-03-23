#include <string.h>
#include <algorithm>
#include <vector>
#include <benchmark/benchmark.h>

namespace {

constexpr int N = 1000;

// Results
// =======
// With optimizations enabled both memcpy() and std::copy() are equal
// speed, with optimizations disabled memcpy() is however about twice
// as fast, the effect gets bigger the smaller the copied array.
//
// Benchmark             Time           CPU Iterations
// ---------------------------------------------------
// bm_memcpy           120 ns        120 ns    5916050
// bm_stdcopy          198 ns        197 ns    3551372
// bm_stdcopy_n        218 ns        216 ns    3314729

void bm_memcpy(benchmark::State& state)
{
  std::vector<int> a(N);
  std::vector<int> r(N);

  while (state.KeepRunning())
  {
    memcpy(r.data(), a.data(), N * sizeof(int));
  }
}

void bm_stdcopy(benchmark::State& state)
{
  std::vector<int> a(N);
  std::vector<int> r(N);

  while (state.KeepRunning())
  {
    std::copy(a.begin(), a.end(), r.begin());
  }
}

void bm_stdcopy_n(benchmark::State& state)
{
  std::vector<int> a(N);
  std::vector<int> r(N);

  while (state.KeepRunning())
  {
    std::copy_n(a.begin(), N, r.begin());
  }
}

} // namespace

BENCHMARK(bm_memcpy);
BENCHMARK(bm_stdcopy);
BENCHMARK(bm_stdcopy_n);

BENCHMARK_MAIN();

/* EOF */
