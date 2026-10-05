#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include "int/isqrt.h"

int main(int argc, char* argv[])
{
    Catch::Session session;

    const int parse_result = session.applyCommandLine(argc, argv);
    if (parse_result != 0)
    {
        return parse_result;
    }

    // Hidden [!benchmark] cases are skipped unless selected; default to them
    // when the user did not pass an explicit test spec.
    auto& tests_or_tags = session.configData().testsOrTags;
    if (tests_or_tags.empty())
    {
        tests_or_tags.emplace_back("[!benchmark]");
    }

    return session.run();
}

TEST_CASE("integerSqrt vs float sqrt over 1000 samples", "[!benchmark]")
{
    constexpr int sample_count = 1000;
    constexpr int warmup_count = 10;

    std::mt19937 rng{0xB441355B};
    std::uniform_int_distribution<std::int32_t> dist{
        0,
        std::numeric_limits<std::int32_t>::max()};

    std::vector<std::int32_t> values(static_cast<std::size_t>(sample_count));

    for (int i = 0; i < sample_count; ++i)
    {
        values[static_cast<std::size_t>(i)] = dist(rng);
    }

    // Float path includes int->float conversion: inputs are integers in the
    // realistic comparison against integerSqrt.
    BENCHMARK_ADVANCED("int-to-float + std::sqrt x1000")(Catch::Benchmark::Chronometer meter)
    {
        volatile float warmup_sink = 0.0f;
        for (int i = 0; i < warmup_count; ++i)
        {
            warmup_sink = std::sqrt(
                static_cast<float>(values[static_cast<std::size_t>(i)]));
        }
        (void)warmup_sink;

        meter.measure([&]
        {
            float sum = 0.0f;
            for (int i = 0; i < sample_count; ++i)
            {
                sum += std::sqrt(
                    static_cast<float>(values[static_cast<std::size_t>(i)]));
            }
            return sum;
        });
    };

    BENCHMARK_ADVANCED("integerSqrt x1000")(Catch::Benchmark::Chronometer meter)
    {
        volatile int warmup_sink = 0;
        for (int i = 0; i < warmup_count; ++i)
        {
            warmup_sink = integerSqrt(values[static_cast<std::size_t>(i)]).p_;
        }
        (void)warmup_sink;

        meter.measure([&]
        {
            int sum = 0;
            for (int i = 0; i < sample_count; ++i)
            {
                sum += integerSqrt(values[static_cast<std::size_t>(i)]).p_;
            }
            return sum;
        });
    };

    // Bit-by-bit integer sqrt used only for compile-time table generation.
    BENCHMARK_ADVANCED("detail::constexprSqrt x1000")(Catch::Benchmark::Chronometer meter)
    {
        volatile std::uint64_t warmup_sink = 0;
        for (int i = 0; i < warmup_count; ++i)
        {
            warmup_sink = detail::constexprSqrt(
                static_cast<std::uint64_t>(values[static_cast<std::size_t>(i)]));
        }
        (void)warmup_sink;

        meter.measure([&]
        {
            std::uint64_t sum = 0;
            for (int i = 0; i < sample_count; ++i)
            {
                sum += detail::constexprSqrt(
                    static_cast<std::uint64_t>(values[static_cast<std::size_t>(i)]));
            }
            return sum;
        });
    };
}
