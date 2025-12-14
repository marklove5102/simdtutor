#include "config.h"

void foo()
{
#if USE_TRHEE && USE_BABY
    puts("I'm three baby");
#elif USE_TRHEE && !USE_BABY
    puts("I'm three body");
#endif
}
