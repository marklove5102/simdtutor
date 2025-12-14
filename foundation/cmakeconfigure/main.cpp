#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>

int main()
{
    chdir(CMAKE_SOURCE_DIR);

#if USE_BABY
    puts("baby mode");
    int age = 3;
#else
    puts("adult mode");
    int age = 18;
#endif
    printf("your age is %d\n", age);

    std::ifstream fin("a.txt");
    if (!fin) {
        printf("not found: a.txt\n");
        return 1;
    }
    std::string str;
    fin >> str;
    printf("%s\n", str.c_str());
    std::system("ls");
}
