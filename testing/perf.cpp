#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>
#include <vector>

#include "int/isqrt.h"

namespace
{

using Clock = std::chrono::steady_clock;

#if defined(_MSC_VER)
#define NUMLIB_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define NUMLIB_NOINLINE __attribute__((noinline))
#else
#define NUMLIB_NOINLINE
#endif

// Keep the timed body in its own function so the hot loop is easy to inspect
// in asm and stays clear of the timing harness.
NUMLIB_NOINLINE float runFloatSqrtBatch(
    const std::int32_t* values,
    std::size_t count) noexcept
{
    float sum = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += std::sqrt(static_cast<float>(values[i]));
    }

    return sum;
}

NUMLIB_NOINLINE int runIntegerSqrtBatch(
    const std::int32_t* values,
    std::size_t count) noexcept
{
    int sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += integerSqrt(values[i]).p_;
    }

    return sum;
}

NUMLIB_NOINLINE std::uint64_t runConstexprSqrtBatch(
    const std::int32_t* values,
    std::size_t count) noexcept
{
    std::uint64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += detail::constexprSqrt(static_cast<std::uint64_t>(values[i]));
    }

    return sum;
}

template <typename Fn>
double timeBatchNs(Fn&& fn, int warmup_runs, int sample_runs)
{
    for (int i = 0; i < warmup_runs; ++i)
    {
        volatile auto sink = fn();
        (void)sink;
    }

    std::vector<double> samples(static_cast<std::size_t>(sample_runs));

    for (int i = 0; i < sample_runs; ++i)
    {
        const auto start = Clock::now();
        volatile auto sink = fn();
        const auto end = Clock::now();
        (void)sink;

        samples[static_cast<std::size_t>(i)] =
            std::chrono::duration<double, std::nano>(end - start).count();
    }

    std::sort(samples.begin(), samples.end());
    return samples[samples.size() / 2];
}

void printResult(
    std::string_view name,
    double batch_ns,
    int sample_count,
    double sink)
{
    const double per_call_ns = batch_ns / static_cast<double>(sample_count);

    std::cout << std::left << std::setw(36) << name
              << " median " << std::setw(10) << std::fixed << std::setprecision(1)
              << batch_ns << " ns/batch  "
              << std::setw(8) << per_call_ns << " ns/call"
              << "  (sink=" << sink << ")\n";
}

} // namespace

int main()
{
    constexpr int sample_count = 1000;
    constexpr int warmup_runs = 20;
    constexpr int sample_runs = 200;

    std::mt19937 rng{0xB441355B};
    std::uniform_int_distribution<std::int32_t> dist{
        0,
        std::numeric_limits<std::int32_t>::max()};

    std::vector<std::int32_t> values(static_cast<std::size_t>(sample_count));

    for (int i = 0; i < sample_count; ++i)
    {
        values[static_cast<std::size_t>(i)] = dist(rng);
    }

    const std::int32_t* data = values.data();
    const std::size_t count = values.size();

    std::cout << "numlib perf  samples=" << sample_count
              << "  warmup_runs=" << warmup_runs
              << "  timed_runs=" << sample_runs
              << "  (median of timed runs)\n";

    {
        float sink = 0.0f;
        const double ns = timeBatchNs(
            [&]
            {
                sink = runFloatSqrtBatch(data, count);
                return sink;
            },
            warmup_runs,
            sample_runs);
        printResult("int-to-float + std::sqrt", ns, sample_count, sink);
    }

    {
        int sink = 0;
        const double ns = timeBatchNs(
            [&]
            {
                sink = runIntegerSqrtBatch(data, count);
                return sink;
            },
            warmup_runs,
            sample_runs);
        printResult("integerSqrt", ns, sample_count, sink);
    }

    {
        std::uint64_t sink = 0;
        const double ns = timeBatchNs(
            [&]
            {
                sink = runConstexprSqrtBatch(data, count);
                return sink;
            },
            warmup_runs,
            sample_runs);
        printResult("detail::constexprSqrt", ns, sample_count, static_cast<double>(sink));
    }

    return 0;
}
