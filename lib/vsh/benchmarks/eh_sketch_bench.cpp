#include <benchmark/benchmark.h>
#include <vsh/eh_sketch.hpp>
#include <array>

static void BM_EhSketchCount(benchmark::State& state) {
    std::array<std::byte, 1 << 20> buffer;
    std::pmr::monotonic_buffer_resource local_pool{
        buffer.data(), buffer.size()
    };
    vsh::EHSketch eh{&local_pool};  
    
    for (int i = 0; i < state.range(0); i++) {
        eh.TickIncrement();
    }

    for (auto _ : state) {
        benchmark::DoNotOptimize(eh.Count());
    }
}

BENCHMARK(BM_EhSketchCount) -> Range(1, 1'000'000);

