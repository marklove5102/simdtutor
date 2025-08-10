#include <fmt/format.h>
#include <fmt/ranges.h>
#include <thread>
#include "show_time.h"
#include <smmintrin.h>


template <class T, size_t N, bool AtomicWait = false>
struct spsc_ring
{
    alignas(64) std::atomic<T *> m_write_pos;
    alignas(64) std::atomic<T *> m_read_pos;
    alignas(64) T *m_write_pos_cached;
    T *m_read_pos_local;
    alignas(64) T *m_read_pos_cached;
    T *m_write_pos_local;

    alignas(64) T m_ring_buffer[N];

    spsc_ring()
        : m_write_pos{m_ring_buffer}
        , m_read_pos{m_ring_buffer}
        , m_write_pos_cached{m_ring_buffer}
        , m_read_pos_local{m_ring_buffer}
        , m_read_pos_cached{m_ring_buffer}
        , m_write_pos_local{m_ring_buffer}
    {}

    void write(const T *input_first, const T *input_last)
    {
        T *write_pos_local = m_write_pos_local;
        T *read_pos_cached = m_read_pos_cached;
        while (input_first != input_last) {
            T *next_write_pos = write_pos_local;
            ++next_write_pos;
            if (next_write_pos == m_ring_buffer + N) {
                next_write_pos = m_ring_buffer;
            }
            if (next_write_pos == read_pos_cached) {
                while (true) {
                    read_pos_cached = m_read_pos.load(std::memory_order_acquire);
                    if (next_write_pos != read_pos_cached) {
                        break;
                    }
                    m_write_pos.store(write_pos_local, std::memory_order_release);
#if __cpp_lib_atomic_wait
                    if constexpr (AtomicWait) {
                        m_write_pos.notify_one();
                        m_read_pos.wait(read_pos_cached, std::memory_order_acquire);
                    }
#endif
                }
            }
            *write_pos_local = *input_first;
            ++input_first;
            write_pos_local = next_write_pos;
        }
        m_write_pos.store(write_pos_local, std::memory_order_release);
#if __cpp_lib_atomic_wait
        if constexpr (AtomicWait) {
            m_write_pos.notify_one();
        }
#endif
        m_write_pos_local = write_pos_local;
        m_read_pos_cached = read_pos_cached;
    }

    void read(T *output_first, T *output_last)
    {
        T *read_pos_local = m_read_pos_local;
        T *write_pos_cached = m_write_pos_cached;
        while (output_first != output_last) {
            if (read_pos_local == write_pos_cached) {
                while (true) {
                    write_pos_cached = m_write_pos.load(std::memory_order_acquire);
                    if (read_pos_local != write_pos_cached) {
                        break;
                    }
                    m_read_pos.store(read_pos_local, std::memory_order_release);
#if __cpp_lib_atomic_wait
                    if constexpr (AtomicWait) {
                        m_read_pos.notify_one();
                        m_write_pos.wait(write_pos_cached, std::memory_order_acquire);
                    }
#endif
                }
            }
            *output_first = *read_pos_local;
            ++output_first;
            ++read_pos_local;
            if (read_pos_local == m_ring_buffer + N) {
                read_pos_local = m_ring_buffer;
            }
        }
        m_read_pos.store(read_pos_local, std::memory_order_release);
#if __cpp_lib_atomic_wait
        if constexpr (AtomicWait) {
            m_read_pos.notify_one();
        }
#endif
        m_read_pos_local = read_pos_local;
        m_write_pos_cached = write_pos_cached;
    }

    const T *write_some(const T *input_first, const T *input_last)
    {
        T *write_pos_local = m_write_pos_local;
        T *read_pos_cached = m_read_pos_cached;
        while (input_first != input_last) {
            T *next_write_pos = write_pos_local;
            ++next_write_pos;
            if (next_write_pos == m_ring_buffer + N) {
                next_write_pos = m_ring_buffer;
            }
            if (next_write_pos == read_pos_cached) {
                read_pos_cached = m_read_pos.load(std::memory_order_acquire);
                if (next_write_pos != read_pos_cached) {
                    break;
                }
            }
            *write_pos_local = *input_first;
            ++input_first;
            write_pos_local = next_write_pos;
        }
        m_write_pos.store(write_pos_local, std::memory_order_release);
#if __cpp_lib_atomic_wait
        if constexpr (AtomicWait) {
            m_write_pos.notify_one();
        }
#endif
        m_write_pos_local = write_pos_local;
        m_read_pos_cached = read_pos_cached;
        return input_first;
    }

    T *read_some(T *output_first, T *output_last)
    {
        T *read_pos_local = m_read_pos_local;
        T *write_pos_cached = m_write_pos_cached;
        while (output_first != output_last) {
            if (read_pos_local == write_pos_cached) {
                write_pos_cached = m_write_pos.load(std::memory_order_acquire);
                if (read_pos_local != write_pos_cached) {
                    break;
                }
            }
            *output_first = *read_pos_local;
            ++output_first;
            ++read_pos_local;
            if (read_pos_local == m_ring_buffer + N) {
                read_pos_local = m_ring_buffer;
            }
        }
        m_read_pos.store(read_pos_local, std::memory_order_release);
#if __cpp_lib_atomic_wait
        if constexpr (AtomicWait) {
            m_read_pos.notify_one();
        }
#endif
        m_read_pos_local = read_pos_local;
        m_write_pos_cached = write_pos_cached;
        return output_first;
    }
};


spsc_ring<int, 1024, false> ring;

const int N = 5 * 1024 * 1024;

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
        ring.read(buf, buf + B);
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
