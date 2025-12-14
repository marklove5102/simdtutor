#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#if fmt_FOUND
#include <fmt/format.h>
#endif
#if TBB_FOUND
#include <tbb/parallel_for.h>
#endif

int main()
{
#if USE_BABY
    puts("baby mode");
    int age = 3;
#else
    puts("adult mode");
    int age = 18;
#endif
    printf("your age is %d\n", age);

    std::ifstream fin(CMAKE_SOURCE_DIR "/a.txt");
    if (!fin) {
        printf("not found: a.txt\n");
        return 1;
    }
    std::string str;
    fin >> str;
    printf("%s\n", str.c_str());

#if fmt_FOUND
    fmt::print("hello, world\n");
#else
    printf("hello, world\n");
#endif

#if TBB_FOUND
    tbb::parallel_for(0, 10, [&] (int i) {
#else
    #pragma omp parallel for
    for (int i = 0; i < 10; ++i) {
#endif
        printf("%d\n", i);
#if TBB_FOUND
    });
#else
    }
#endif

#if XXX_FOUND
    xxx::func();
#endif
}
