#include <fmt/format.h>
#include <fmt/ranges.h>
#include <thread>
#include "show_time.h"
#include "spsc_ring.h"


spsc_ring<int, 1024, false> ring;

const int N = 1024 * 1024;

void producer()
{
    const int B = 512;
    static int buf[B];
    for (int i = 0; i < B; ++i) {
        buf[i] = i;
    }
    for (int j = 0; j < N / B; ++j) {
        ring.write(buf, buf + B);
    }
}

void consumer()
{
    const int B = 512;
    static int buf[B];
    for (int j = 0; j < N / B; ++j) {
        for (int *p = buf, *pe = buf + B; p != pe;) {
            p = ring.read_some(p, pe);
        }
        for (int i = 0; i < B; ++i) {
            if (buf[i] != i) [[unlikely]] {
                fmt::println("Data Error: {} != {}", buf[i], i);
                exit(1);
            }
        }
    }
}

int main()
{
    for (int i = 0; i < 100; ++i) {
        show_time _("queue");
        std::jthread producer_thread(producer);
        std::jthread consumer_thread(consumer);
        producer_thread.join();
        consumer_thread.join();
    }
}
