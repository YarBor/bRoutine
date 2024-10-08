#include "b_routine_env.h"

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <sys/time.h>
bRoutineEnv::TaskDistributor_t* bRoutineEnv::TaskDistributor_t::New(int size)
{
    bRoutineEnv::TaskDistributor_t* i = (bRoutineEnv::TaskDistributor_t*)calloc(1, sizeof(bRoutineEnv::TaskDistributor_t));
    i->ActiveTasks = (TaskItemsList**)calloc(size, sizeof(TaskItemsList*));
    i->schedulers = (bScheduler**)calloc(size, sizeof(bScheduler*));
    // 在 registe 的时候增长
    i->size = 0;
    return i;
}
void bRoutineEnv::TaskDistributor_t::AddTask(TaskItem* task)
{
    static uint i = 0;
    if (task->self->IsProgress) {
        throw("Task commit Wrong");
    }
    this->ActiveTasks[i]->pushBack(task);
    if ((!this->schedulers[i]->IsThreadRunning) && !pthread_mutex_trylock(&this->schedulers[i]->mutex)) {
        pthread_cond_signal(&this->schedulers[i]->cond);
        pthread_mutex_unlock(&this->schedulers[i]->mutex);
    }
    i = (i + 1) % this->size;
}
// struct bRoutineEnv *bRoutineEnv::get()
int bRoutineEnv::TaskDistributor_t::registe(bScheduler* i)
{
    static pthread_mutex_t lock;
    pthread_mutex_lock(&lock);
    this->schedulers[this->size] = i;
    this->ActiveTasks[this->size] = i->ActivetaskItems;
    int a = size;
    size += 1;
    pthread_mutex_unlock(&lock);
    return a;
}
struct bRoutine* bRoutineEnv::bRoutinePool_t::pop()
{
    bRoutine* i = nullptr;
    pthread_mutex_lock(&this->mutex);
    if (this->DoneRoutines.size() != 0) {
        i = this->DoneRoutines.pop();
    }
    pthread_mutex_unlock(&this->mutex);
    return i;
}
void bRoutineEnv::bRoutinePool_t::push(bRoutine* r)
{
    pthread_mutex_lock(&this->mutex);
    this->DoneRoutines.push(r);
    pthread_mutex_unlock(&this->mutex);
}
struct bRoutineStack* bRoutineEnv::bUnSharedStackPool_t::pop(int level)
{
    bRoutineStack* i = nullptr;
    switch (level) {
    case StackLevel::MiniStack:
        pthread_mutex_lock(&this->mutexMini);
        if (this->DoneRoutinesMiniStacks.size() != 0) {
            i = this->DoneRoutinesMiniStacks.pop();
        }
        pthread_mutex_unlock(&this->mutexMini);
        break;
    case StackLevel::MediumStack:
        pthread_mutex_lock(&this->mutexMedium);
        if (this->DoneRoutinesMediumStacks.size() != 0) {
            i = this->DoneRoutinesMediumStacks.pop();
        }
        pthread_mutex_unlock(&this->mutexMedium);
        break;
    case StackLevel::LargeStack:
        pthread_mutex_lock(&this->mutexLarge);
        if (this->DoneRoutinesLargeStacks.size() != 0) {
            i = this->DoneRoutinesLargeStacks.pop();
        }
        pthread_mutex_unlock(&this->mutexLarge);
        break;
    default:
        break;
    }
    if (i)
        RoutineNums--;
    //   pthread_mutex_lock(&this->mutex);
    //   if (!this->DoneRoutinesStacks.empty()) {
    //     i = this->DoneRoutinesStacks.front();
    //     this->DoneRoutinesStacks.pop_front();
    //   }
    //   pthread_mutex_unlock(&this->mutex);
    return i;
}
int bRoutineEnv::bUnSharedStackPool_t::push(struct bRoutineStack* bst)
{
    //   pthread_mutex_lock(&this->mutex);
    //   this->DoneRoutinesStacks.emplace_back(bst);
    //   pthread_mutex_unlock(&this->mutex);
    if (RoutineNums >= Max_Keep_Routines_Thread) {
        return -1;
    }
    switch (bst->StackSize / 1024) {
    case bRoutineStack::LargeStackSize:
        pthread_mutex_lock(&this->mutexLarge);
        this->DoneRoutinesMiniStacks.push(bst);
        pthread_mutex_unlock(&this->mutexLarge);
        break;
    case bRoutineStack::MediumStackSize:
        pthread_mutex_lock(&this->mutexMedium);
        this->DoneRoutinesMediumStacks.push(bst);
        pthread_mutex_unlock(&this->mutexMedium);
        break;
    case bRoutineStack::MiniStackSize:
        pthread_mutex_lock(&this->mutexMini);
        this->DoneRoutinesMiniStacks.push(bst);
        pthread_mutex_unlock(&this->mutexMini);
        break;
    default:
        break;
    }
    return ++RoutineNums;
}

static bRoutineEnv* __bRoutineEnv = 0;

struct bRoutineEnv* InitializeBRoutineEnv(int RunProcessNums)
{
    bRoutineEnv* New__bRoutineEnv = NULL;
    if (New__bRoutineEnv)
        return New__bRoutineEnv;
    New__bRoutineEnv = (bRoutineEnv*)calloc(1, sizeof(struct bRoutineEnv));
    New__bRoutineEnv->TaskDistributor = bRoutineEnv::TaskDistributor_t::New(RunProcessNums);
    // New__bRoutineEnv->TaskDistributor->size = RunProcessNums;
    // New__bRoutineEnv->ActiveTasks = TaskItemsList::New();
    New__bRoutineEnv->RoutinePool = bRoutineEnv::bRoutinePool_t::New();
    New__bRoutineEnv->UnSharedStackPool = bRoutineEnv::bUnSharedStackPool_t::New();
    New__bRoutineEnv->Epoll = bEpoll::New();
    //   New__bRoutineEnv->schedulers = (decltype(New__bRoutineEnv->schedulers))calloc(
    //   New__bRoutineEnv->runingProgressNum, sizeof(bScheduler *));

    return New__bRoutineEnv;
}

void* ProcessBegin(void* i);

void CreatMutProcessEnv(bRoutineEnv* New__bRoutineEnv)
{
    bScheduler* thisSch = bScheduler::get();
    thisSch->occupyRoutine->id = 1;
    // New__bRoutineEnv->TaskDistributor->schedulers[0] = thisSch;
    // 挂起主线程
    auto threadArg = (threadArgs*)calloc(1, sizeof(threadArgs));
    bRoutine* mainR = bRoutine::New(StackLevel::LargeStack, ProcessBegin, threadArg);
    mainR->id = 0;
    mainR->IsEnableHook = true;
    thisSch->pendingRoutine = mainR;
    thisSch->pendingRoutine->IsProgress = true;
    thisSch->occupyRoutine->IsMain = true;
    thisSch->SwapContext();
    // 切换上下文
    for (int i = 1; i < New__bRoutineEnv->TaskDistributor->size; ++i) {
        auto threadArg = (threadArgs*)calloc(1, sizeof(threadArgs));
        pthread_create(&threadArg->thread, NULL, ProcessBegin, threadArg);
        pthread_detach(threadArg->thread);
        // 起物理线程
        // 初始化物理线程的调度者
    }
}
struct bRoutineEnv* bRoutineEnv::init(uint RunProcessNums)
{
    if (__bRoutineEnv == NULL) {
        static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&lock);
        if (__bRoutineEnv == NULL) {
            __bRoutineEnv = InitializeBRoutineEnv(RunProcessNums);
            CreatMutProcessEnv(__bRoutineEnv);
            // __bRoutineEnv = New__bRoutineEnv;
        }
        pthread_mutex_unlock(&lock);
    }
    return __bRoutineEnv;
}

struct bRoutineEnv* bRoutineEnv::get()
{
    if (__bRoutineEnv == NULL)
        return bRoutineEnv::init(defaultProcess);
    return __bRoutineEnv;
};
struct bRoutineEnv::bRoutinePool_t* bRoutineEnv::bRoutinePool_t::New()
{
    struct bRoutinePool_t* i = (struct bRoutinePool_t*)calloc(1, sizeof(struct bRoutinePool_t));
    i->mutex = PTHREAD_MUTEX_INITIALIZER;
    return i;
}
struct bRoutineEnv::bUnSharedStackPool_t* bRoutineEnv::bUnSharedStackPool_t::New()
{
    struct bUnSharedStackPool_t* i = (struct bUnSharedStackPool_t*)calloc(1, sizeof(struct bUnSharedStackPool_t));
    i->mutexLarge = PTHREAD_MUTEX_INITIALIZER;
    i->mutexMedium = PTHREAD_MUTEX_INITIALIZER;
    i->mutexMini = PTHREAD_MUTEX_INITIALIZER;
    return i;
}
void bRoutineEnv::Deal()
{
    // DebugPrint("bRoutineEnv::Deal Called\n");
    int i = epoll_wait(this->Epoll->epollFd, this->Epoll->Revents->eventArr, this->Epoll->Revents->eventSize, 0);
    if (i == -1) {
        throw("epoll return false Erron :>" + std::to_string(errno));
    } else {
        if (i)
            DebugPrint("EpollWait Return %d\n", i);
        auto& arr = this->Epoll->Revents->eventArr;
        TaskItem* task;
        for (int q = 0; q < i; q++) {
            if (((TaskItem*)arr[q].data.ptr)->perpareFuncPtr)
                ((TaskItem*)arr[q].data.ptr)->perpareFuncPtr(((TaskItem*)arr[q].data.ptr)->perpareFuncArgs, arr + q);
        }
        for (int q = 0; q < i; q++) {
            if (((TaskItem*)arr[q].data.ptr)->callbackFuncPtr)
                ((TaskItem*)arr[q].data.ptr)->callbackFuncPtr(((TaskItem*)arr[q].data.ptr)->callbackFunArgs);
        }
        // for (int q = 0; q < i; q++) {
        //     task = (TaskItem*)arr[q].data.ptr;
        //     epoll_ctl(this->Epoll->epollFd, EPOLL_CTL_DEL, task->epollFd, &task->bEpollEvent);
        // }
        task = nullptr;
        auto timeoutTaskList = this->Epoll->TimeoutScaner->NewMergeUpdate2Now();
        if (timeoutTaskList->ItemCount.load())
            DebugPrint("epoll Timer merge %d task\n", timeoutTaskList->ItemCount.load());
        while ((task = timeoutTaskList->popHead()) != nullptr) {
            if (task->timeoutCallFunc)
                task->timeoutCallFunc(task->timeoutCallFuncArgs);
            task->IsTasktimeout = 1;
            this->TaskDistributor->AddTask(task);
        }
        TaskItemsList::Delete(timeoutTaskList);
    }
    this->Epoll->lastCheckTime = bRoutineEnv::Timer::getNow_Ms();
}
void bRoutineEnv::bRoutineRecycle(bRoutine*& rt)
{
    rt->IsDone = 1;
    if (rt->IsMain)
        return;
    if (this->UnSharedStackPool->push(rt->stack) == -1) {
        auto i = bScheduler::get();
        i->IsPendingNeedDelete = 1;
        return;
        // i->SwapContext();
    };
    rt->stack = nullptr;
    rt->context->stackPtr = nullptr;
    rt->context->stackSize = 0;

    this->RoutinePool->push(rt);
    rt = nullptr;
}
long long getMicroseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long microseconds = (long long)tv.tv_sec * 1000000LL + (long long)tv.tv_usec;
    return microseconds;
}
long long getMilliseconds()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long milliseconds = (long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL;
    return milliseconds;
}

long long getNanoseconds()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    long long nanoseconds = (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
    return nanoseconds;
}
long long getSeconds()
{
    time_t currentTime = time(NULL);
    return (long long)currentTime;
}
unsigned long long bRoutineEnv::Timer::getNow_S()
{
    return getSeconds();
}
unsigned long long bRoutineEnv::Timer::getNow_Ms()
{
    return getMilliseconds();
}
unsigned long long bRoutineEnv::Timer::getNow_Us()
{
    return getMicroseconds();
}
unsigned long long bRoutineEnv::Timer::getNow_Ns()
{
    return getNanoseconds();
}
// 不能偷main 容易造成内存上下文切换异常
TaskItem* Try2stealTask(TaskItemsList* activeTask)
{
    TaskItem* a = nullptr;

    if (!activeTask->ItemCount.load()) {
        return 0; // 任务数量为零，直接返回
    }

    bool headLocked = pthread_mutex_trylock(activeTask->HeadMutex) == 0;
    if (!headLocked) {
        return 0; // 无法锁定头部互斥锁，直接返回
    }

    if (activeTask->ItemCount.load() < 2) {
        if (activeTask->ItemCount.load() == 0) {
            pthread_mutex_unlock(activeTask->HeadMutex);
            return 0; // 任务数量为零，解锁头部互斥锁后返回
        }

        bool tailLocked = pthread_mutex_trylock(activeTask->TailMutex) == 0;
        if (!tailLocked) {
            pthread_mutex_unlock(activeTask->HeadMutex);
            return 0; // 无法锁定尾部互斥锁，解锁头部互斥锁后返回
        }

        // 执行任务弹出操作
        a = activeTask->popHeadUnsafe();
        if (a->self->IsMain) {
            activeTask->pushBackUnsafe(a);
        }
        pthread_mutex_unlock(activeTask->TailMutex);
    } else {
        // 执行任务弹出操作
        a = activeTask->popHeadUnsafe();
        if (a->self->IsMain) {
            pthread_mutex_lock(activeTask->TailMutex);
            activeTask->pushBackUnsafe(a);
            pthread_mutex_unlock(activeTask->HeadMutex);
            pthread_mutex_unlock(activeTask->TailMutex);
            return nullptr;
        }
    }
    pthread_mutex_unlock(activeTask->HeadMutex);
    return a;
}

struct TaskItem* bRoutineEnv::stealTaskRoutine(bScheduler* sch)
{
    auto i = this->TaskDistributor;
    for (int id = (sch->ID + 1) % i->size; id != sch->ID; ++id) {
        auto p = Try2stealTask(i->ActiveTasks[id]);
        if (p)
            sch->ActivetaskItems->pushBack(p);
    }
    return sch->ActivetaskItems->popHead();
}
