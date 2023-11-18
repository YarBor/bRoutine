#pragma once

#include <pthread.h>
#include <sys/epoll.h>

#include "bRoutine.h"
// #include "bRoutineEnv.h"

#include <atomic>
#define TIMEWAITSECONDS_S 30
#define BEpollMaxEventsSize 1e5
struct bEpTimer {
    static inline struct bEpTimer* New();
    TaskItemsList** TaskItemBucket;
    unsigned long long TaskItemBucketSize;
    //
    /*这俩是一起变的*/
    unsigned long long StartTime;
    unsigned long long BucketPosNow;
    //
    TaskItemsList* getBucket(unsigned long long ms_want);
    TaskItemsList* NewMergeUpdate2Now();
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
    TaskItemsList* TimeOutTaskList;
    TaskItemsList* ActiveTaskList;
    epollRevents* Revents;
    static inline bEpoll* New();
    std::atomic_ullong lastCheckTime;
};

template <typename T>
T inline* RemoveSelfFromOwnerlink(T* i);

// 控制线程 去处理 超时 和 协程active 塞到 env 的 active 链中
void EpollDeal(bEpoll* i)
{
    ;
}