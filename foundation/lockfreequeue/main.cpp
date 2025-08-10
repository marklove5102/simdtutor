#include <fmt/format.h>
#include <thread>
#include "mt_queue.h"
#include "show_time.h"



// SPSC - single producer single consumer
// SPMC - single producer multiple consumer
// MPSC - multiple producer single consumer
// MPMC - multiple producer multiple consumer


alignas(64) int ring[1024]; // 环形队列
alignas(64) std::atomic_int write_pos; // 跨CPU共享，互相访问，速度=L3
alignas(64) std::atomic_int read_pos;
alignas(64) int write_pos_local;       // 单个CPU专用，不和人共享的，速度=L1
alignas(64) int read_pos_local;
alignas(64) int write_pos_cached;
alignas(64) int read_pos_cached;

// 0           4          8                 12             16  --------- 64
// write_pos | read_pos | write_pos_local* | read_pos_local | .asdasdasdasdasdasd. |
// 0           4          8                                   64                                   128
// write_pos | read_pos | .................................. | write_pos_local | ................. | read_pos_local | .................... | int ginista

// atomic: true-sharing: 1000yuan
// false-sharing:        1000yuan (shiji 1yuan)

//  P      C
// L1*    L1
// L2     L2
//    L3+

// "std::atomic" = atomicness + memoryorder

// "std::atomic" relaxed = atomicness
// "std::atomic" acquire = atomicness + memoryorder (如果别人用 release 写入了这个 atomic，那么我的 acquire atomic 之后，的所有内存操作，都会在别人 release 前的操作后发生）

// CPU 追求高效，自己会擅自重排序，但是呢，他可以保证，执行的结果不变

// mov eax, 1
// mov ecx, 2
// eax = 1, ecx = 2

// 重排OK
// mov ecx, 2
// mov eax, 1
// eax = 1, ecx = 2

// mov eax, 1
// mov ecx, eax
// eax = 1, ecx = 1

// 如果重排（不允许！所以不会重排）
// mov ecx, eax
// mov eax, 1
// eax = 1, ecx = 0

// 编译器也会重排
// struct S {
// atomic_int i;
// atomic_int j;
// };
// S.j = 2;
// S.i = 1;

// movdq [S], 0x000000001000000002 NONONONONONO

// riscv CPU: 哎呀！编译器生成了lock前缀！这是说他的用户不想重排内存访问顺序！所以激进如我riscv CPU也不敢重排！
// lock mov [S+4], 0x000000002
// lock mov [S], 0x000000001

// x86 CPU: 我本来就不重排内存 mov
// mov [S+4], 0x000000002
// mov [S], 0x000000001

// x86 Load-Store

// memory order:
// seq_cst
// acq_rel
// acquire
// release
// relaxed

// int data;
// std::atomic_bool has_data;
//
// void prod() { // CPU0
//     data = 42; // CPU0 L1*
//     // mov [data], 42
//     // Store-Store
//     // mov [has_data], true
//     // bao*mfence
//     has_data.store(true, std::memory_order_release); // 大障壁
//     // ****buyaosfence
//     // 等待 data 写入操作完成到 L3 的转移，CPU0 L1 -> L3*
//     // 才能继续把 has_data 真正设为 true
//
//     // data = 0
//     // data = 42
//     // has_data = true
// }
//
// void cons() { // CPU1
//     // again:
//     // mov eax, [has_data]
//     // cmp eax, true
//     // je again
//     // Load-Load
//     // 如果离开这个循环，则 data 42 一定已经生效
//     // mov [data], 42
//     while (has_data.load(std::memory_order_acquire) != true); // 大障壁
//     // 等待其他线程在 has_data 写入前的任何操作（比如 data = 42）完成后，has_data 才能返回 true
//     // has_data == true
//     // read data
//     fmt::print("data is {}", data); // 一定能打出 42
// }

// cache-line: 64Bytes

void ring_push(int *buf, size_t n)
{
    int w = write_pos_local;
    int next_write_pos = (w + 1) % 1024;
    while (next_write_pos == read_pos_cached) {
        write_pos.store(w, std::memory_order_release);
        read_pos_cached = read_pos.load(std::memory_order_acquire);
    }
    ring[w] = value;
    write_pos_local = next_write_pos;
    write_pos.store(write_pos_local, std::memory_order_release);
}

int ring_pop()
{
    int r = read_pos_local;
    while (r == write_pos_cached) {
        read_pos.store(r, std::memory_order_release);
        write_pos_cached = write_pos.load(std::memory_order_acquire);
    }
    int value = ring[r];
    read_pos_local = (r + 1) % 1024;
    read_pos.store(read_pos_local, std::memory_order_release);
    return value;
}


const int N = 5 * 1024 * 1024; // 500万个int数据

void producer() // 生产者线程
{
    for (int i = 0; i < N; ++i) {
        ring_push(i);
    }
}

void consumer() // 消费者线程
{
    for (int i = 0; i < N; ++i) {
        int value = ring_pop();
        if (value != i) {
            fmt::println("Data Error: {} != {}", value, i);
            exit(1);
        }
    }
}

int main()
{
    show_time _("queue");
    std::jthread producer_thread(producer);
    std::jthread consumer_thread(consumer);
    producer_thread.join();
    consumer_thread.join();
}
