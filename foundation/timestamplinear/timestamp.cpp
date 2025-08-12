#include <cstdlib>
#include <vector>
#include <cstdint>
#include <random>
#include <algorithm>
#include <fmt/format.h>
#include <immintrin.h>
#include "show_time.h"


[[gnu::noinline]] int32_t timestampLinear(int32_t timestamp)
{
    int32_t hours = timestamp / 10000000;
    int32_t minutes = (timestamp / 100000) % 100;
    int32_t seconds = (timestamp / 1000) % 100;
    int32_t milliseconds = timestamp % 1000;

    return (hours * 3600 + minutes * 60 + seconds) * 1000 + milliseconds;
}

[[gnu::noinline]] int32_t timestampLinear_V2(int32_t timestamp)
{
    int32_t hours = timestamp / 10000000;
    int32_t minutes = (timestamp / 100000) % 100;
    int32_t milliseconds = timestamp % 100000;

    return hours * 3600000 + minutes * 60000 + milliseconds;
}

[[gnu::noinline]] int32_t timestampLinear_V3(int32_t timestamp)
{
    uint32_t u = timestamp;

    uint32_t hours = u / 10000000U;
    uint32_t minutes = (u / 100000U) % 100U;
    uint32_t milliseconds = u % 100000U;

    return hours * 3600000U + minutes * 60000U + milliseconds;
}

[[gnu::noinline]] int32_t timestampLinear_V4(int32_t timestamp)
{
    uint32_t u = timestamp;

    // uint32_t hours = u / 10000000U;
    // uint32_t minutes = (u / 100000U) % 100U;
    // uint32_t milliseconds = u % 100000U;

    uint16_t mh = uint16_t((uint64_t(u >> 5) * 175921861) >> 39);
    uint16_t hours = uint16_t((uint32_t(mh >> 2) * 5243) >> 17);
    uint16_t minutes = mh - hours * 100;
    uint32_t milliseconds = u - mh * 100000;

    return uint32_t(hours) * 3600000 + uint32_t(minutes) * 60000 + milliseconds;
}

[[gnu::noinline]] int32_t timestampLinear_V5(int32_t timestamp)
{
    uint32_t u = timestamp;

    uint16_t mh = uint16_t((uint64_t(u >> 5) * 175921861) >> 39);
    uint16_t hours = uint16_t((uint32_t(mh >> 2) * 5243) >> 17);
    return u - uint32_t(hours * 60 + mh) * 40000;
}

[[gnu::noinline]] int32_t timestampDelinear(int32_t time)
{
    int32_t milliseconds = time % 1000;
    time /= 1000;
    int32_t seconds = time % 60;
    time /= 60;
    int32_t minutes = time % 60;
    time /= 60;
    int32_t hours = time % 24;
    return milliseconds + 1000 * (seconds + 100 * (minutes + 100 * hours));
}

[[gnu::noinline]] int32_t timestampDelinear_V2(int32_t time)
{
    int32_t milliseconds = time % 60000;
    time /= 60000;
    int32_t minutes = time % 60;
    time /= 60;
    int32_t hours = time;
    return milliseconds + 100000 * (minutes + 100 * hours);
}

[[gnu::noinline]] int32_t timestampDelinear_V3(int32_t time)
{
    uint32_t u = time;
    uint16_t mh = uint16_t((uint64_t(u) * 1172812403) >> 46);
    uint16_t hours = uint16_t((uint32_t(mh) * 34953) >> 21);
    return u + 40000 * uint32_t(mh + 100 * hours);
}


void randomize_data(int32_t *a, size_t n)
{
    std::uniform_int_distribution<int32_t> u60{0, 59};
    std::uniform_int_distribution<int32_t> u1000{0, 999};
    std::mt19937 rng;
    std::generate(a, a + n, [&] () mutable {
        char buf[10];
        fmt::format_to(buf, "{:02d}{:02d}{:02d}{:03d}", u60(rng), u60(rng), u60(rng), u1000(rng));
        buf[9] = 0;
        return std::atoi(buf);
    });
}


template <int32_t (*conv)(int32_t)>
void convert(int32_t *a, int32_t *b, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        a[i] = conv(a[i]);
    }
}


int main()
{
    std::vector<int32_t> ini(1000000);
    randomize_data(ini.data(), ini.size());

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampLinear  ", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampLinear>(a.data(), b.data(), a.size());
            }
        }
    }

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampLinear_V2", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampLinear_V2>(a.data(), b.data(), a.size());
            }
        }
    }

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampLinear_V3", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampLinear_V3>(a.data(), b.data(), a.size());
            }
        }
    }

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampLinear_V4", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampLinear_V4>(a.data(), b.data(), a.size());
            }
        }
    }

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampLinear_V5", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampLinear_V5>(a.data(), b.data(), a.size());
            }
        }
    }

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampDelinear", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampDelinear>(a.data(), b.data(), a.size());
            }
        }
    }

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampDelinear_V2", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampDelinear_V2>(a.data(), b.data(), a.size());
            }
        }
    }

    {
        std::vector<int32_t> a = ini;
        std::vector<int32_t> b(a.size());
        {
            show_time _("timestampDelinear_V3", 1000000 * 10);
            for (int i = 0; i < 10; ++i) {
                convert<timestampDelinear_V3>(a.data(), b.data(), a.size());
            }
        }
    }

    fmt::println("{}", timestampDelinear(timestampLinear_V5(235959999)));
    fmt::println("{}", timestampDelinear_V3(timestampLinear(235959999)));
    // fmt::println("{}", timestampLinear_V4(145659100));
}
