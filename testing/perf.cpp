#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <cpuid.h>
#endif
#endif

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

NUMLIB_NOINLINE std::uint64_t runUint64SqrtBatch(
    const std::uint64_t* values,
    std::size_t count) noexcept
{
    std::uint64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += integerSqrt(values[i]).p_;
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
        sum += values[i].sinCordic().getRawValue();
    }

    return static_cast<float>(sum);
}

NUMLIB_NOINLINE float runFixedSinLutBatch(
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

NUMLIB_NOINLINE float runSinTurnsBatch(
    const std::uint32_t* turns,
    std::size_t count) noexcept
{
    std::int64_t sum = 0;

    for (std::size_t i = 0; i < count; ++i)
    {
        sum += sinTurns(turns[i]).getRawValue();
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

struct BenchResult
{
    std::string name;
    double batch_ns = 0.0;
    double per_call_ns = 0.0;
    double sink = 0.0;
};

void printResult(const BenchResult& result)
{
    std::cout << std::left << std::setw(36) << result.name
              << " median " << std::setw(10) << std::fixed << std::setprecision(1)
              << result.batch_ns << " ns/batch  "
              << std::setw(8) << result.per_call_ns << " ns/call"
              << "  (sink=" << result.sink << ")\n";
}

template <typename Fn>
void runNamed(
    std::vector<BenchResult>& results,
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

    BenchResult row;
    row.name = std::string{name};
    row.batch_ns = ns;
    row.per_call_ns = ns / static_cast<double>(sample_count);
    row.sink = static_cast<double>(sink);
    printResult(row);
    results.push_back(std::move(row));
}

std::string jsonEscape(std::string_view text)
{
    std::string out;
    out.reserve(text.size());
    for (const char ch : text)
    {
        switch (ch)
        {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += ch;
            break;
        }
    }
    return out;
}

std::string trimAscii(std::string text)
{
    const auto not_space = [](unsigned char ch) { return ch != ' ' && ch != '\0'; };
    while (!text.empty() && !not_space(static_cast<unsigned char>(text.front())))
    {
        text.erase(text.begin());
    }
    while (!text.empty() && !not_space(static_cast<unsigned char>(text.back())))
    {
        text.pop_back();
    }
    return text;
}

std::string detectOs()
{
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "unknown";
#endif
}

std::string detectCpu()
{
    char brand[49] = {};

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    int regs[4] = {};
    __cpuid(regs, static_cast<int>(0x80000000u));
    if (static_cast<unsigned>(regs[0]) >= 0x80000004u)
    {
        for (unsigned i = 0; i < 3; ++i)
        {
            __cpuid(regs, static_cast<int>(0x80000002u + i));
            std::memcpy(brand + i * 16, regs, 16);
        }
        return trimAscii(brand);
    }
#elif (defined(__GNUC__) || defined(__clang__)) && \
    (defined(__x86_64__) || defined(__i386__))
    unsigned eax = 0;
    unsigned ebx = 0;
    unsigned ecx = 0;
    unsigned edx = 0;
    if (__get_cpuid_max(0x80000000u, nullptr) >= 0x80000004u)
    {
        for (unsigned i = 0; i < 3; ++i)
        {
            __get_cpuid(0x80000002u + i, &eax, &ebx, &ecx, &edx);
            std::memcpy(brand + i * 16 + 0, &eax, 4);
            std::memcpy(brand + i * 16 + 4, &ebx, 4);
            std::memcpy(brand + i * 16 + 8, &ecx, 4);
            std::memcpy(brand + i * 16 + 12, &edx, 4);
        }
        return trimAscii(brand);
    }
#endif

#if defined(__linux__)
    std::ifstream cpuinfo{"/proc/cpuinfo"};
    std::string line;
    while (std::getline(cpuinfo, line))
    {
        constexpr std::string_view key = "model name";
        if (line.compare(0, key.size(), key) == 0)
        {
            const auto colon = line.find(':');
            if (colon != std::string::npos)
            {
                return trimAscii(line.substr(colon + 1));
            }
        }
    }
#endif

    return "unknown";
}

std::string isoUtcTimestamp()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::optional<std::filesystem::path> findRepoRoot()
{
    namespace fs = std::filesystem;
    fs::path dir = fs::current_path();
    for (int i = 0; i < 8; ++i)
    {
        if (fs::is_regular_file(dir / "CMakeLists.txt") &&
            fs::is_directory(dir / "docs"))
        {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent == dir)
        {
            break;
        }
        dir = parent;
    }
    return std::nullopt;
}

bool writePerfResultsJson(
    const std::filesystem::path& path,
    int sample_count,
    int warmup_runs,
    int sample_runs,
    const std::vector<BenchResult>& results)
{
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (!out)
    {
        std::cerr << "failed to write " << path.string() << '\n';
        return false;
    }

    out << std::setprecision(17);
    out << "{\n";
    out << "  \"meta\": {\n";
    out << "    \"samples\": " << sample_count << ",\n";
    out << "    \"warmup_runs\": " << warmup_runs << ",\n";
    out << "    \"timed_runs\": " << sample_runs << ",\n";
    out << "    \"os\": \"" << jsonEscape(detectOs()) << "\",\n";
    out << "    \"cpu\": \"" << jsonEscape(detectCpu()) << "\",\n";
    out << "    \"captured_at\": \"" << jsonEscape(isoUtcTimestamp()) << "\",\n";
    out << "    \"note\": \"Bare-metal baseline; Release perf writes this file\"\n";
    out << "  },\n";
    out << "  \"benchmarks\": [\n";

    for (std::size_t i = 0; i < results.size(); ++i)
    {
        const BenchResult& row = results[i];
        out << "    {\n";
        out << "      \"name\": \"" << jsonEscape(row.name) << "\",\n";
        out << "      \"batch_ns\": " << row.batch_ns << ",\n";
        out << "      \"per_call_ns\": " << row.per_call_ns << ",\n";
        out << "      \"sink\": " << row.sink << "\n";
        out << "    }";
        if (i + 1 < results.size())
        {
            out << ',';
        }
        out << '\n';
    }

    out << "  ]\n";
    out << "}\n";
    return static_cast<bool>(out);
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

    std::mt19937_64 rng64{0xB441355Bull};
    std::uniform_int_distribution<std::uint64_t> dist64{
        0u,
        std::numeric_limits<std::uint64_t>::max()};

    std::vector<std::int32_t> values(static_cast<std::size_t>(sample_count));
    std::vector<std::uint64_t> values64(static_cast<std::size_t>(sample_count));
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
    std::vector<std::uint32_t> turns(static_cast<std::size_t>(sample_count));

    for (int i = 0; i < sample_count; ++i)
    {
        const std::size_t index = static_cast<std::size_t>(i);
        values[index] = dist(rng);
        values64[index] = dist64(rng64);
        turns[index] = static_cast<std::uint32_t>(dist64(rng64));

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
    std::vector<BenchResult> results;
    results.reserve(16);

    std::cout << "numlib perf  samples=" << sample_count
              << "  warmup_runs=" << warmup_runs
              << "  timed_runs=" << sample_runs
              << "  (median of timed runs)\n\n";

    std::cout << "integer sqrt\n";
    runNamed(
        results,
        "int-to-float + std::sqrt",
        [&]
        {
            return runFloatSqrtBatch(data, count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "integerSqrt",
        [&]
        {
            return runIntegerSqrtBatch(data, count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "detail::constexprSqrt",
        [&]
        {
            return static_cast<double>(runConstexprSqrtBatch(data, count));
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "integerSqrt uint64",
        [&]
        {
            return static_cast<double>(
                runUint64SqrtBatch(values64.data(), count));
        },
        warmup_runs,
        sample_runs,
        sample_count);

    std::cout << "\nFixed16 vs float (pre-converted inputs)\n";
    runNamed(
        results,
        "float add",
        [&]
        {
            return runFloatAddBatch(float_a.data(), float_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 add",
        [&]
        {
            return runFixedAddBatch(fixed_a.data(), fixed_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        results,
        "float mul",
        [&]
        {
            return runFloatMulBatch(float_a.data(), float_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 mul",
        [&]
        {
            return runFixedMulBatch(fixed_a.data(), fixed_b.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        results,
        "float div",
        [&]
        {
            return runFloatDivBatch(float_a.data(), float_divisor.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 div",
        [&]
        {
            return runFixedDivBatch(fixed_a.data(), fixed_divisor.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        results,
        "float mul-add",
        [&]
        {
            return runFloatMulAddBatch(float_a.data(), float_b.data(), float_c.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 mulAdd",
        [&]
        {
            return runFixedMulAddBatch(fixed_a.data(), fixed_b.data(), fixed_c.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        results,
        "float sqrt",
        [&]
        {
            return runFloatFixedSqrtBatch(float_positive.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 sqrt",
        [&]
        {
            return runFixedSqrtBatch(fixed_positive.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    runNamed(
        results,
        "float sin",
        [&]
        {
            return runFloatSinBatch(float_angle.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 sin (CORDIC)",
        [&]
        {
            return runFixedSinBatch(fixed_angle.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 sin (LUT, FixedN result)",
        [&]
        {
            return runFixedSinLutBatch(fixed_angle.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "sinTurns (LUT, FixedN result)",
        [&]
        {
            return runSinTurnsBatch(turns.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);
    runNamed(
        results,
        "Fixed16 sinViaFloat",
        [&]
        {
            return runFixedSinViaFloatBatch(fixed_angle.data(), count);
        },
        warmup_runs,
        sample_runs,
        sample_count);

    const auto repo_root = findRepoRoot();
    if (!repo_root)
    {
        std::cerr << "could not find repo root (CMakeLists.txt + docs/) from "
                  << std::filesystem::current_path().string() << '\n';
        return 1;
    }

    const std::filesystem::path results_path =
        *repo_root / "docs" / "perf-results.json";
    if (!writePerfResultsJson(
            results_path,
            sample_count,
            warmup_runs,
            sample_runs,
            results))
    {
        return 1;
    }

    std::cout << "\nwrote " << results_path.string() << '\n';
    return 0;
}
