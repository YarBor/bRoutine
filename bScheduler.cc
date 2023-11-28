// #include "bScheduler.h"
#include "bContext.h"
#include "bRoutineEnv.h"
#include <stdlib.h>
#ifdef TEST_SWAP_TIME
unsigned long long testSwapTime;
uint64_t get_timestamp()
{
    uint32_t low, high;
    asm volatile("rdtsc" : "=a" (low), "=d" (high));
    return (static_cast<uint64_t>(high) << 32) | low;
}
#endif
static __thread struct bScheduler* scheduler = 0;
bScheduler* bSchedulerCreat()
{
    auto i = (struct bScheduler*)calloc(1, sizeof(struct bScheduler));
    i->ActivetaskItems = TaskItemsList::New();
    // 这个occuR...就是 调度/主函数 (单线程时 是主函数 ,其他工作线程是调度函数)
    i->occupyRoutine = bRoutine::Alloc();
    i->occupyRoutine->IsEnableHook = 1;
    i->occupyRoutine->id = 0;
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
    auto pc = this->occupyRoutine->context;
    auto oc = this->pendingRoutine->context;
    std::swap(this->occupyRoutine, this->pendingRoutine);
    DebugPrint("%d Routines wake up\n", this->occupyRoutine->id);
#ifdef TEST_SWAP_TIME
    testSwapTime = get_timestamp();
#endif
    bSwap(pc, oc);
#ifdef TEST_SWAP_TIME
    auto p = get_timestamp();
    printf("Routine swap used %lld ns\n", p - testSwapTime);
    testSwapTime = p;
#endif
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
        if ((now - Env->Epoll->lastCheckTime.load())) {
            if (!pthread_mutex_trylock(&Env->bEpollDealLock)) {
                Env->Deal();
                pthread_mutex_unlock(&Env->bEpollDealLock);
            } else {
                DebugPrint("try lock Env->bEpollDealLock false \n");
            }
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
                    if (Scheduler->pendingRoutine->IsMain == false)
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
