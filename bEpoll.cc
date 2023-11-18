#include <stdlib.h>
#include <string.h>

#include <cassert>

#include "bRoutineEnv.h"

struct bEpTimer* bEpTimer::New()
{
    struct bEpTimer* i = (struct bEpTimer*)calloc(1, sizeof(struct bEpTimer));
    i->BucketPosNow = 0;
    i->TaskItemBucketSize = 1000 * TIMEWAITSECONDS_S;
    i->TaskItemBucket = (TaskItemsList**)calloc(i->TaskItemBucketSize, sizeof(TaskItemsList*));
    for (unsigned int a = 0; a < i->TaskItemBucketSize; a++)
        i->TaskItemBucket[a] = TaskItemsList::New();
    i->StartTime = bRoutineEnv::Timer::getNow_Ms();
}

bEpoll* bEpoll::New()
{
    bEpoll* i = (bEpoll*)calloc(1, sizeof(bEpoll));
    i->epollFd = epoll_create(1);
    i->ActiveTaskList = TaskItemsList::New();
    i->TimeOutTaskList = TaskItemsList::New();
    i->Revents = (epollRevents*)calloc(1, sizeof(epollRevents));
    i->Revents->eventSize = BEpollMaxEventsSize;
    i->Revents->eventArr = (struct epoll_event*)calloc(i->Revents->eventSize, sizeof(struct epoll_event));
    i->mtx = PTHREAD_MUTEX_INITIALIZER;
    i->lastCheckTime = bRoutineEnv::Timer::getNow_Ms();
    return i;
}
TaskItemsList* bEpTimer::getBucket(unsigned long long ms_want)
{
    if (ms_want > this->TaskItemBucketSize) {
        ms_want = this->TaskItemBucketSize;
    }
    return this->TaskItemBucket[(this->BucketPosNow + ms_want - this->StartTime) % this->TaskItemBucketSize];
}
TaskItemsList* bEpTimer::NewMergeUpdate2Now()
{
    auto result = TaskItemsList::New();
    auto now_ms = bRoutineEnv::Timer::getNow_Ms();
    auto target_pos = (now_ms - this->StartTime + this->BucketPosNow) % this->TaskItemBucketSize;
    for (auto i = this->BucketPosNow + 1; i <= target_pos; i = (i + 1) % this->TaskItemBucketSize) {
        this->TaskItemBucket[i]->pushAll2(result);
    }
    this->BucketPosNow = target_pos;
    this->StartTime = now_ms;
    return result;
}
