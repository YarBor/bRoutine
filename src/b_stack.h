#pragma once

#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include "b_common.h"
typedef unsigned int uint;
// Stack Size
enum StackLevel {
    // 128KB
    MiniStack = 1,
    // 1MB
    MediumStack,
    // 4MB
    LargeStack
};
struct bRoutineStack {
    /* data */

    enum StackSize_Kb {
        MiniStackSize = MiniStackSize__,
        MediumStackSize = MediumStackSize__,
        LargeStackSize = LargeStackSize__
    };
    static struct bRoutineStack* New(int stackSizeLevel);
    static void Del(bRoutineStack*);
    static uint GetPageSize_b()
    {
        static uint pageSize = getpagesize();
        return pageSize;
    }
    // need unmap to free
    void* StackPtr;
    size_t StackSize;
};
