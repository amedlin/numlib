#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string_view>
#include <type_traits>
#include <vector>

#include "fixedpt/fixed_types.h"
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

NUMLIB_NOINLINE float runFloatAddBatch(
    const float* a,
    const float* b,
    std::size_t count) noexcept
{
    float sum = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += a[i] + b[i];
    }

    return sum;
}

NUMLIB_NOINLINE float runFixedAddBatch(
    const Fixed16* a,
    const Fixed16* b,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += (a[i] + b[i]).getRawValue();
    }

    return static_cast<float>(sum);
}

NUMLIB_NOINLINE float runFloatMulBatch(
    const float* a,
    const float* b,
    std::size_t count) noexcept
{
    float sum = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += a[i] * b[i];
    }

    return sum;
}

NUMLIB_NOINLINE float runFixedMulBatch(
    const Fixed16* a,
    const Fixed16* b,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += (a[i] * b[i]).getRawValue();
    }

    return static_cast<float>(sum);
}

NUMLIB_NOINLINE float runFloatDivBatch(
    const float* a,
    const float* b,
    std::size_t count) noexcept
{
    float sum = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += a[i] / b[i];
    }

    return sum;
}

NUMLIB_NOINLINE float runFixedDivBatch(
    const Fixed16* a,
    const Fixed16* b,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += (a[i] / b[i]).getRawValue();
    }

    return static_cast<float>(sum);
}

NUMLIB_NOINLINE float runFloatMulAddBatch(
    const float* a,
    const float* b,
    const float* c,
    std::size_t count) noexcept
{
    float sum = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += a[i] * b[i] + c[i];
    }

    return sum;
}

NUMLIB_NOINLINE float runFixedMulAddBatch(
    const Fixed16* a,
    const Fixed16* b,
    const Fixed16* c,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += mulAdd(a[i], b[i], c[i]).getRawValue();
    }

    return static_cast<float>(sum);
}

NUMLIB_NOINLINE float runFloatFixedSqrtBatch(
    const float* values,
    std::size_t count) noexcept
{
    float sum = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += std::sqrt(values[i]);
    }

    return sum;
}

NUMLIB_NOINLINE float runFixedSqrtBatch(
    const Fixed16* values,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += values[i].sqrt().getRawValue();
    }

    return static_cast<float>(sum);
}

NUMLIB_NOINLINE float runFloatSinBatch(
    const float* values,
    std::size_t count) noexcept
{
    float sum = 0.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += std::sin(values[i]);
    }

    return sum;
}

NUMLIB_NOINLINE float runFixedSinBatch(
    const Fixed16* values,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += values[i].sin().getRawValue();
    }

    return static_cast<float>(sum);
}

NUMLIB_NOINLINE float runFixedSinViaFloatBatch(
    const Fixed16* values,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += values[i].sinViaFloat().getRawValue();
    }

    return static_cast<float>(sum);
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

template <typename Fn>
void runNamed(
    std::string_view name,
    Fn&& fn,
    int warmup_runs,
    int sample_runs,
    int sample_count)
{
    using Result = std::decay_t<decltype(fn())>;
    Result sink{};
    const double ns = timeBatchNs(
        [&]
        {
            sink = fn();
            return sink;
        },
        warmup_runs,
        sample_runs);
    printResult(name, ns, sample_count, static_cast<double>(sink));
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

    // Values stay inside Fixed16 range for add/mul/div/sqrt/sin.
    std::uniform_real_distribution<float> fixed_dist{-50.0f, 50.0f};
    std::uniform_real_distribution<float> positive_dist{0.25f, 50.0f};
    std::uniform_real_distribution<float> divisor_dist{0.5f, 25.0f};
    std::uniform_real_distribution<float> angle_dist{-8.0f, 8.0f};

    std::vector<std::int32_t> values(static_cast<std::size_t>(sample_count));
    std::vector<float> float_a(static_cast<std::size_t>(sample_count));
    std::vector<float> float_b(static_cast<std::size_t>(sample_count));
    std::vector<float> float_c(static_cast<std::size_t>(sample_count));
    std::vector<float> float_positive(static_cast<std::size_t>(sample_count));
    std::vector<float> float_divisor(static_cast<std::size_t>(sample_count));
    std::vector<float> float_angle(static_cast<std::size_t>(sample_count));
    std::vector<Fixed16> fixed_a(static_cast<std::size_t>(sample_count));
    std::vector<Fixed16> fixed_b(static_cast<std::size_t>(sample_count));
    std::vector<Fixed16> fixed_c(static_cast<std::size_t>(sample_count));
    std::vector<Fixed16> fixed_positive(static_cast<std::size_t>(sample_count));
    std::vector<Fixed16> fixed_divisor(static_cast<std::size_t>(sample_count));
    std::vector<Fixed16> fixed_angle(static_cast<std::size_t>(sample_count));

    for (int i = 0; i < sample_count; ++i)
    {
        const std::size_t index = static_cast<std::size_t>(i);
        values[index] = dist(rng);

        float_a[index] = fixed_dist(rng);
        float_b[index] = fixed_dist(rng);
        float_c[index] = fixed_dist(rng);
        float_positive[index] = positive_dist(rng);
        float_divisor[index] = divisor_dist(rng);
        float_angle[index] = angle_dist(rng);

        fixed_a[index] = Fixed16{float_a[index]};
        fixed_b[index] = Fixed16{float_b[index]};
        fixed_c[index] = Fixed16{float_c[index]};
        fixed_positive[index] = Fixed16{float_positive[index]};
        fixed_divisor[index] = Fixed16{float_divisor[index]};
        fixed_angle[index] = Fixed16{float_angle[index]};
    }

    const std::int32_t* data = values.data();
    const std::size_t count = values.size();

    std::cout << "numlib perf  samples=" << sample_count
              << "  warmup_runs=" << warmup_runs
              << "  timed_runs=" << sample_runs
              << "  (median of timed runs)\n\n";

    std::cout << "integer sqrt\n";
    runNamed(
        "int-to-float + std::sqrt",
        [&]
        {
            return runFloatSqrtBatch(data, count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "integerSqrt",
        [&]
        {
            return runIntegerSqrtBatch(data, count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "detail::constexprSqrt",
        [&]
        {
            return static_cast<double>(runConstexprSqrtBatch(data, count));
        },
        warmup_runs,
        sample_runs,
        sample_count);

    std::cout << "\nFixed16 vs float (pre-converted inputs)\n";
    runNamed(
        "float add",
        [&]
        {
            return runFloatAddBatch(float_a.data(), float_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "Fixed16 add",
        [&]
        {
            return runFixedAddBatch(fixed_a.data(), fixed_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        "float mul",
        [&]
        {
            return runFloatMulBatch(float_a.data(), float_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "Fixed16 mul",
        [&]
        {
            return runFixedMulBatch(fixed_a.data(), fixed_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        "float div",
        [&]
        {
            return runFloatDivBatch(float_a.data(), float_divisor.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "Fixed16 div",
        [&]
        {
            return runFixedDivBatch(fixed_a.data(), fixed_divisor.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        "float mul-add",
        [&]
        {
            return runFloatMulAddBatch(float_a.data(), float_b.data(), float_c.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "Fixed16 mulAdd",
        [&]
        {
            return runFixedMulAddBatch(fixed_a.data(), fixed_b.data(), fixed_c.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        "float sqrt",
        [&]
        {
            return runFloatFixedSqrtBatch(float_positive.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "Fixed16 sqrt",
        [&]
        {
            return runFixedSqrtBatch(fixed_positive.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        "float sin",
        [&]
        {
            return runFloatSinBatch(float_angle.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "Fixed16 sin (CORDIC)",
        [&]
        {
            return runFixedSinBatch(fixed_angle.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        "Fixed16 sinViaFloat",
        [&]
        {
            return runFixedSinViaFloatBatch(fixed_angle.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    return 0;
}
