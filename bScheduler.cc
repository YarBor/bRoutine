// #include "bScheduler.h"

#include <stdlib.h>

#include "bContext.h"
#include "bRoutineEnv.h"
static __thread struct bScheduler* scheduler = 0;
bScheduler* bSchedulerCreat()
{
    auto i = (struct bScheduler*)calloc(1, sizeof(struct bScheduler));
    i->ActivetaskItems = TaskItemsList::New();
    // 这个occuR...就是 调度/主函数
    i->occupyRoutine = bRoutine::Alloc();
    i->occupyRoutine->context = bContext::New();
    i->occupyRoutine->IsProgress = true;
    i->occupyRoutine->IsBegin = true;
    i->pendingRoutine = NULL;
    i->hangTimeOut = HangTimeout;
    return i;
}

struct bScheduler* bScheduler::get()
{
    if (!scheduler) {
        static pthread_mutex_t mutex; // 互斥锁
        pthread_mutex_lock(&mutex);
        if (!scheduler) {
            scheduler = bSchedulerCreat();
        }
        pthread_mutex_unlock(&mutex);
    }
    return scheduler;
}
void bScheduler::SwapContext()
{
    if (this->occupyRoutine->IsMain && this->occupyRoutine->IsProgress) {
        auto i = TaskItem::New();
        i->self = this->occupyRoutine;
        this->occupyRoutine->IsProgress = false;
        this->ActivetaskItems->pushBack(i);
    }
    std::swap(this->occupyRoutine, this->pendingRoutine);
    bSwap(this->pendingRoutine->context, this->occupyRoutine->context);
}

void* RoutineBegin(void* args)
{
    auto i = (bRoutine*)args;
    if (i->IsBegin) {
        throw("return Wrong");
    } else {
        i->IsBegin = true;
        i->func(i->args);
        i->IsDone = true;
        i->IsBegin = false;
        free(i->args);
        i->args = nullptr;
        // 复用
        bRoutineEnv::get()->bRoutineRecycle(i);

        bScheduler::get()->SwapContext();
    }
    return nullptr;
}

void* ProcessBegin(void* i)
{
    auto args = (struct threadArgs*)i;
    auto Scheduler = bScheduler::get();
    auto Env = bRoutineEnv::get();
    Scheduler->ID = Env->TaskDistributor->registe(Scheduler);
    Scheduler->occupyRoutine->IsProgress = true;
    struct TaskItem* task = NULL;
    unsigned long long now;
    while (1) {
        now = bRoutineEnv::Timer::getNow_Ms();
        if ((now - Env->Epoll->lastCheckTime.load()) && pthread_mutex_trylock(&Env->bEpollDealLock)) {
            Env->Deal();
            pthread_mutex_unlock(&Env->bEpollDealLock);
        }

        task = Scheduler->ActivetaskItems->popHead();
        if (task == nullptr)
            task = Env->stealTaskRoutine(Scheduler);
        if (task) {
            if (task->self) {
                // 上下文切换
                Scheduler->pendingRoutine = task->self;
                Scheduler->SwapContext();
                if (Scheduler->IsPendingNeedDelete) {
                    bRoutine::Del(Scheduler->pendingRoutine);
                    Scheduler->IsPendingNeedDelete = 0;
                }
            }
            TaskItem::Delete(task);
        } else {
            pthread_mutex_lock(&Scheduler->mutex);
            Scheduler->IsThreadRunning = 0;
            pthread_cond_timedwait(&Scheduler->cond, &Scheduler->mutex, &Scheduler->hangTimeOut);
            Scheduler->IsThreadRunning = 1;
            pthread_mutex_unlock(&Scheduler->mutex);
        }
        if (Scheduler->IsStop)
            break;
    }
    free(args);
    return 0;
}
