#pragma once
#define MiniStackSize__ 128
#define MediumStackSize__ 1024
#define LargeStackSize__ 4096
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
    static uint GetPageSize_b();
    // need unmap to free
    void* StackPtr;
    size_t StackSize;
};
