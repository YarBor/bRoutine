#include "bStack.h"

#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>
using Stack = struct bRoutineStack;

void* allocateStackMemory(size_t size_Kb)
{
    if (size_Kb < Stack::StackSize_Kb::MiniStackSize) {
        size_Kb = Stack::StackSize_Kb::MiniStackSize;
    }
    char* mem;
    /*                                                 前后保护页  */
    if ((mem = (char*)mmap(NULL, (size_Kb)*1024 + 2 * Stack::GetPageSize_b(), PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS | MAP_PRIVATE, -1, 0))
            == MAP_FAILED
        || mprotect(mem, Stack::GetPageSize_b(), PROT_NONE) != 0 || mprotect(mem + (size_Kb)*1024 + Stack::GetPageSize_b(), Stack::GetPageSize_b(), PROT_NONE) != 0) {
        throw("Mmap SysCall Failed");
    }
    return mem;
}

struct bRoutineStack* bRoutineStack::New(int sizeLevel)
{
    bRoutineStack* ret = (bRoutineStack*)calloc(1, sizeof(bRoutineStack));
    if (sizeLevel == MiniStack) {
        ret->StackPtr = allocateStackMemory(Stack::StackSize_Kb::MiniStackSize);
        ret->StackSize = Stack::StackSize_Kb::MiniStackSize * 1024;
    } else if (sizeLevel == MediumStack) {
        ret->StackPtr = allocateStackMemory(Stack::StackSize_Kb::MediumStackSize);
        ret->StackSize = Stack::StackSize_Kb::MediumStackSize * 1024;
    } else if (sizeLevel == LargeStack) {
        ret->StackPtr = allocateStackMemory(Stack::StackSize_Kb::LargeStackSize);
        ret->StackSize = Stack::StackSize_Kb::LargeStackSize * 1024;
    } else {
        throw("Undefined stack size" + std::to_string(sizeLevel));
        return nullptr;
    }
    return ret;
}

uint bRoutineStack::GetPageSize_b()
{
    static uint pageSize = getpagesize();
    return pageSize;
}