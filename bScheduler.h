#pragma once

#include <pthread.h>
#include <time.h>

#include "bRoutine.h"
#include "common.h"
#include "bStack.h"

#define OneMsAsNs 1000000
#define HangTimeout                                                                                 \
    {                                                                                               \
        SchedulerHangTimeout_S, long(SchedulerHangTimeout_Ms * OneMsAsNs) + SchedulerHangTimeout_Ns \
    }
struct bScheduler {
    //   bRoutineStack *SharedStack;
    bRoutine* pendingRoutine;
    bRoutine* occupyRoutine;
    struct TaskItemsList* ActivetaskItems;
    int ID;
    char IsThreadRunning;
    char IsStop;
    char IsPendingNeedDelete;
    pthread_cond_t cond; // 条件变量
    pthread_mutex_t mutex; // 互斥锁
    timespec hangTimeOut;
    static struct bScheduler* get();
    void SwapContext();
};
struct threadArgs {
    pthread_t thread;
    int threadId;
};
void* RoutineBegin(void* args);
void* ProcessBegin(void* i);
