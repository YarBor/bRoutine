#pragma once
// #include <sys/epoll.h>


#include "bContext.h"
#include "bStack.h"

typedef void*(RoutineFunc)(void*);
struct bRoutine {
    /* data */
    RoutineFunc* func;
    void* args;
    char IsMain, IsBegin, IsDone, IsHangup, IsProgress;
    bRoutineStack* stack;
    struct bContext* context;
    static bRoutine* New(int StackLevel, RoutineFunc* func, void* args);
    static bRoutine* Alloc();
    //   inline int Init(int StackLevel, RoutineFunc *func,void * args);
    void Resume();
    static void yield();
};
