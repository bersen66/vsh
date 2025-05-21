#include <benchmark/benchmark.h>
#include <vsh/eh_sketch.hpp>

static void BM_EhSketchCount(benchmark::State& state) {
    vsh::EHSketch eh;  
    
    for (int i = 0; i < state.range(0); i++) {
        eh.TickIncrement();
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(eh.Count());
    }
}

BENCHMARK(BM_EhSketchCount) -> Range(1, 1'000'000);
