#pragma once


#include <chrono>
#include <fmt/format.h>


struct show_time
{
    const char *name;
    std::chrono::steady_clock::time_point t0;
    size_t niters;

    show_time(const char *name, int niters = 1)
        : name(name), t0(std::chrono::steady_clock::now()), niters(niters)
    {
    }

    ~show_time()
    {
        auto t1 = std::chrono::steady_clock::now();
        double sec = std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
        sec /= niters;
        if (sec < 0.00'000'1) {
            fmt::println("{}\t: {:.3f}ns", name, sec * 1000'000'000);
        } else if (sec < 0.00'1) {
            fmt::println("{}\t: {:.5f}us", name, sec * 1000'000);
        } else if (sec < 1.0) {
            fmt::println("{}\t: {:.7f}ms", name, sec * 1000);
        } else {
            fmt::println("{}\t: {:.7f}s", name, sec);
        }
    }
};
