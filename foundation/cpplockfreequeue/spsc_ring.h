#pragma once

#include <atomic>
#include <cstddef>


template <class T, size_t N, bool AtomicWait = false>
struct alignas(64) spsc_ring
{
    static_assert(std::atomic<T *>::is_always_lock_free, "atomic pointer not lock-free");

    alignas(64) std::atomic<T *> m_write_pos;
    alignas(64) std::atomic<T *> m_read_pos;

    alignas(64) T *m_write_pos_cached;
    T *m_read_pos_local;

    alignas(64) T *m_read_pos_cached;
    T *m_write_pos_local;

    alignas(64) T m_ring_buffer[N];

    spsc_ring(spsc_ring &&) = delete;

    spsc_ring()
        : m_write_pos{m_ring_buffer}
        , m_read_pos{m_ring_buffer}
        , m_write_pos_cached{m_ring_buffer}
        , m_read_pos_local{m_ring_buffer}
        , m_read_pos_cached{m_ring_buffer}
        , m_write_pos_local{m_ring_buffer}
    {}

    template <class InputIt, class InputIte>
    InputIt write(InputIt input_first, InputIte input_last)
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
        return input_first;
    }

    template <class OutputIt, class OutputIte>
    OutputIt read(OutputIt output_first, OutputIte output_last)
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
        return output_first;
    }

    template <class InputIt, class InputIte>
    InputIt write_some(InputIt input_first, InputIte input_last)
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
                if (next_write_pos == read_pos_cached) {
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

    template <class OutputIt, class OutputIte>
    OutputIt read_some(OutputIt output_first, OutputIte output_last)
    {
        T *read_pos_local = m_read_pos_local;
        T *write_pos_cached = m_write_pos_cached;
        while (output_first != output_last) {
            if (read_pos_local == write_pos_cached) {
                write_pos_cached = m_write_pos.load(std::memory_order_acquire);
                if (read_pos_local == write_pos_cached) {
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

//     size_t write_some(const T *input_data, size_t n_write)
//     {
//         T *write_pos_local = m_write_pos_local;
//         T *next_write_pos = write_pos_local + 1;
//         if (next_write_pos == m_ring_buffer + N) {
//             next_write_pos = 0;
//         }
//         T *read_pos_cached = m_read_pos_cached;
//         size_t max_write = static_cast<size_t>((m_ring_buffer + N) - write_pos_local);
//         size_t free_write = static_cast<size_t>(read_pos_cached - write_pos_local);
//         if (free_write == 0) {
//             while ((read_pos_cached = m_read_pos.load(std::memory_order_acquire)) == next_write_pos) {
//                 m_write_pos.store(write_pos_local, std::memory_order_release);
// #if __cpp_lib_atomic_wait
//                 if constexpr (AtomicWait) {
//                     m_write_pos.notify_one();
//                     m_read_pos.wait(read_pos_cached, std::memory_order_acquire);
//                 }
// #endif
//             }
//             free_write = static_cast<size_t>(read_pos_cached - write_pos_local);
//             // free_write must be big enough?
//         }
//         if (free_write < max_write) {
//             max_write = free_write;
//         }
//         if (n_write > max_write) {
//             n_write = max_write;
//             for (const T *p = write_pos_local; p != m_ring_buffer + N; ++p, ++input_data) {
//                 *p = *input_data;
//             }
//             write_pos_local = m_ring_buffer;
//         } else {
//             for (const T *p = write_pos_local, *pe = write_pos_local + n_write; p != pe; ++p, ++input_data) {
//                 *p = *input_data;
//             }
//             write_pos_local += n_write;
//         }
//         m_write_pos.store(write_pos_local, std::memory_order_release);
// #if __cpp_lib_atomic_wait
//         if constexpr (AtomicWait) {
//             m_write_pos.notify_one();
//         }
// #endif
//         m_write_pos_local = write_pos_local;
//         m_read_pos_cached = read_pos_cached;
//         return n_write;
//     }
//
//     size_t read_some(T *output_data, size_t n_read)
//     {
//         T *read_pos_local = m_read_pos_local;
//         T *write_pos_cached = m_write_pos_cached;
//         size_t max_read = static_cast<size_t>((m_ring_buffer + N) - read_pos_local);
//         size_t free_read = static_cast<size_t>(write_pos_cached - read_pos_local);
//         if (free_read == 0) {
//             while ((write_pos_cached = m_write_pos.load(std::memory_order_acquire)) == read_pos_local) {
//                 m_read_pos.store(read_pos_local, std::memory_order_release);
// #if __cpp_lib_atomic_wait
//                 if constexpr (AtomicWait) {
//                     m_read_pos.notify_one();
//                     m_write_pos.wait(write_pos_cached, std::memory_order_acquire);
//                 }
// #endif
//             }
//             free_read = static_cast<size_t>(write_pos_cached - read_pos_local);
//         }
//         if (free_read < max_read) {
//             max_read = free_read;
//         }
//         if (n_read > max_read) {
//             n_read = max_read;
//             T *read_pos_end = m_ring_buffer + N;
//             while (read_pos_local != read_pos_end) {
//                 *output_data++ = *read_pos_local++;
//             }
//             read_pos_local = m_ring_buffer;
//         } else {
//             T *read_pos_end = read_pos_local + n_read;
//             while (read_pos_local != read_pos_end) {
//                 *output_data++ = *read_pos_local++;
//             }
//         }
//         m_read_pos.store(read_pos_local, std::memory_order_release);
// #if __cpp_lib_atomic_wait
//         if constexpr (AtomicWait) {
//             m_read_pos.notify_one();
//         }
// #endif
//         m_read_pos_local = read_pos_local;
//         m_write_pos_cached = write_pos_cached;
//         return n_read;
//     }
//
//     const T *write_some(const T *input_first, const T *input_last)
//     {
//         T *write_pos_local = m_write_pos_local;
//         T *read_pos_cached = m_read_pos_cached;
//         while (input_first != input_last) {
//             T *next_write_pos = write_pos_local;
//             ++next_write_pos;
//             if (next_write_pos == m_ring_buffer + N) {
//                 next_write_pos = m_ring_buffer;
//             }
//             if (next_write_pos == read_pos_cached) {
//                 read_pos_cached = m_read_pos.load(std::memory_order_acquire);
//                 if (next_write_pos != read_pos_cached) {
//                     break;
//                 }
//             }
//             *write_pos_local = *input_first;
//             ++input_first;
//             write_pos_local = next_write_pos;
//         }
//         m_write_pos.store(write_pos_local, std::memory_order_release);
// #if __cpp_lib_atomic_wait
//         if constexpr (AtomicWait) {
//             m_write_pos.notify_one();
//         }
// #endif
//         m_write_pos_local = write_pos_local;
//         m_read_pos_cached = read_pos_cached;
//         return input_first;
//     }
//
//     T *read_some(T *output_first, T *output_last)
//     {
//         T *read_pos_local = m_read_pos_local;
//         T *write_pos_cached = m_write_pos_cached;
//         while (output_first != output_last) {
//             if (read_pos_local == write_pos_cached) {
//                 write_pos_cached = m_write_pos.load(std::memory_order_acquire);
//                 if (read_pos_local != write_pos_cached) {
//                     break;
//                 }
//             }
//             *output_first = *read_pos_local;
//             ++output_first;
//             ++read_pos_local;
//             if (read_pos_local == m_ring_buffer + N) {
//                 read_pos_local = m_ring_buffer;
//             }
//         }
//         m_read_pos.store(read_pos_local, std::memory_order_release);
// #if __cpp_lib_atomic_wait
//         if constexpr (AtomicWait) {
//             m_read_pos.notify_one();
//         }
// #endif
//         m_read_pos_local = read_pos_local;
//         m_write_pos_cached = write_pos_cached;
//         return output_first;
//     }
};
