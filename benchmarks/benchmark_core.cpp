#include <benchmark/benchmark.h>

#include "course/core.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <vector>

namespace {

std::vector<double> make_prices(std::size_t count) {
    std::vector<double> prices;
    prices.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const double position = static_cast<double>(i);
        prices.push_back(100.0 + 0.01 * static_cast<double>(i % 97)
                         + 0.2 * std::sin(0.17 * position)
                         + 0.5 * std::cos(0.031 * position));
    }
    return prices;
}

bool outputs_agree(const std::vector<double>& reference,
                   const std::vector<double>& candidate) {
    if (reference.size() != candidate.size()) {
        return false;
    }
    for (std::size_t i = 0; i < reference.size(); ++i) {
        const double tolerance = 1e-9 + 1e-11 * std::abs(reference[i]);
        if (!std::isfinite(reference[i]) || !std::isfinite(candidate[i])
            || std::abs(reference[i] - candidate[i]) > tolerance) {
            return false;
        }
    }
    return true;
}

using SmaFunction = std::vector<double> (*)(const std::vector<double>&,
                                           std::size_t);

template <SmaFunction CalculateSma>
void measure_sma(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    const auto window = static_cast<std::size_t>(state.range(1));
    auto prices = make_prices(count);

    // Setup and the comparison finish before benchmark timing starts.
    try {
        const auto reference = course::sma_naive(prices, window);
        const auto candidate = course::sma_rolling(prices, window);
        if (reference.size() != count - window + 1
            || !outputs_agree(reference, candidate)) {
            state.SkipWithError("SMA outputs disagree before timing");
            return;
        }
    } catch (const std::exception& error) {
        state.SkipWithError(error.what());
        return;
    }

    auto* input_data = prices.data();
    for (auto _ : state) {
        (void)_;
        benchmark::DoNotOptimize(input_data);
        benchmark::ClobberMemory();
        auto result = CalculateSma(prices, window);
        auto* result_data = result.data();
        benchmark::DoNotOptimize(result_data);
        benchmark::ClobberMemory();
    }

    // One item is one input price, not one arithmetic operation or output.
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(count));
}

void sma_arguments(benchmark::Benchmark* benchmark) {
    for (const std::int64_t count : {1024, 16384, 65536}) {
        for (const std::int64_t window : {1, 20, 100}) {
            benchmark->Args({count, window});
        }
    }
}

}  // namespace

static void BM_SMA_Naive(benchmark::State& state) {
    measure_sma<course::sma_naive>(state);
}

static void BM_SMA_Rolling(benchmark::State& state) {
    measure_sma<course::sma_rolling>(state);
}

BENCHMARK(BM_SMA_Naive)->Apply(sma_arguments)->ArgNames({"n", "window"});
BENCHMARK(BM_SMA_Rolling)->Apply(sma_arguments)->ArgNames({"n", "window"});

BENCHMARK_MAIN();
