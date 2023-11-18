#pragma once

#include <pthread.h>
#include <time.h>

#include "bRoutine.h"
#include "bStack.h"
#define OneMsAsNs 1000000
#define SchedulerHangTimeout_S 0
#define SchedulerHangTimeout_Ms 0.5
#define SchedulerHangTimeout_Ns 0

#define HangTimeout                                                                                 \
    {                                                                                               \
        SchedulerHangTimeout_S, long(SchedulerHangTimeout_Ms * OneMsAsNs) + SchedulerHangTimeout_Ns \
    }
struct bScheduler {
    int ID;
    //   bRoutineStack *SharedStack;
    bRoutine* pendingRoutine;
    bRoutine* occupyRoutine;
    TaskItemsList* ActivetaskItems;
    char IsThreadRunning;
    char IsStop;
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
