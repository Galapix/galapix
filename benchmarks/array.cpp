#include <iostream>
#include <vector>
#include <benchmark/benchmark.h>

void foo(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  std::vector<int> a(n * n);

  while (state.KeepRunning())
  {
    for(int i = 0; i < n; i++)
      for(int j = 0; j < n; j++)
        a[n * j + i] = i * j;

  }
}

void bar(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  std::vector<int> a(n * n);

  while (state.KeepRunning())
  {
    for(int j = 0; j < n; j++)
      for(int i = 0; i  < n; i++)
        a[n * j + i] = i * j;
  }
}

BENCHMARK(bar)
//->Arg(740)
//->Arg(770);
->Arg(740*10)
->Arg(770*14);

BENCHMARK(foo)
//->Arg(740)
//->Arg(770);
->Arg(740*10)
->Arg(770*14);

BENCHMARK_MAIN();

/* EOF */
