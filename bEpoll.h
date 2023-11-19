#pragma once

#include <pthread.h>
#include <sys/epoll.h>

#include "bRoutine.h"
#include "bTask.h"
// #include "bRoutineEnv.h"

#include <atomic>
#define TIMEWAITSECONDS_S 30
#define BEpollMaxEventsSize 1e5
struct bEpTimer {
    static struct bEpTimer* New();
    struct TaskItemsList** TaskItemBucket;
    unsigned long long TaskItemBucketSize;
    //
    /*这俩是一起变的*/
    unsigned long long StartTime;
    unsigned long long BucketPosNow;
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

template <typename T>
T* RemoveSelfFromOwnerlink(T* i);
