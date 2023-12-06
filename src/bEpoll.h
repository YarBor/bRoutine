#pragma once

#include <pthread.h>
#include <sys/epoll.h>

#include "bRoutine.h"
#include "bTask.hh"
// #include "bRoutineEnv.h"
#include "common.h"
#include <atomic>
struct bEpTimer {
    static struct bEpTimer* New();
    struct TaskItemsList** TaskItemBucket;
    unsigned long long TaskItemBucketSize;
    //
    /*这俩是一起变的*/
    unsigned long long LastCheckTime;
    std::atomic<unsigned long long> BucketPosNow;
    //
    struct TaskItemsList* getBucket(unsigned long long ms_want);
    struct TaskItemsList* NewMergeUpdate2Now();
};
struct epollRevents {
    epoll_event* eventArr;
    int eventSize;
};
struct bEpoll {
    int epollFd;
    int eventCount;
    struct bEpTimer* TimeoutScaner;
    pthread_mutex_t mtx;
    struct TaskItemsList* TimeOutTaskList;
    struct TaskItemsList* ActiveTaskList;
    epollRevents* Revents;
    static bEpoll* New();
    std::atomic_ullong lastCheckTime;
};

