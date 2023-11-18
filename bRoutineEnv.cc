#include "bRoutineEnv.h"

#include <chrono>
#include <errno.h>
#include <stdlib.h>
#include <string>
bRoutineEnv::TaskDistributor_t* bRoutineEnv::TaskDistributor_t::New(int size)
{
    bRoutineEnv::TaskDistributor_t* i = (bRoutineEnv::TaskDistributor_t*)calloc(1, sizeof(bRoutineEnv::TaskDistributor_t));
    i->ActiveTasks = (TaskItemsList**)calloc(size, sizeof(TaskItemsList*));
    i->schedulers = (bScheduler**)calloc(size, sizeof(bScheduler*));
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
    if ((!this->schedulers[i]->IsThreadRunning) && pthread_mutex_trylock(&this->schedulers[i]->mutex)) {
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
    if (!this->DoneRoutines.empty()) {
        i = this->DoneRoutines.front();
        this->DoneRoutines.pop_front();
    }
    pthread_mutex_unlock(&this->mutex);
    return i;
}
void bRoutineEnv::bRoutinePool_t::push(bRoutine* r)
{
    pthread_mutex_lock(&this->mutex);
    this->DoneRoutines.emplace_back(r);
    pthread_mutex_unlock(&this->mutex);
}
struct bRoutineStack* bRoutineEnv::bUnSharedStackPool_t::pop(int level)
{
    bRoutineStack* i = nullptr;
    switch (level) {
    case StackLevel::MiniStack:
        pthread_mutex_lock(&this->mutexMini);
        i = this->DoneRoutinesMiniStacks.front();
        this->DoneRoutinesMiniStacks.pop_front();
        pthread_mutex_unlock(&this->mutexMini);
        break;
    case StackLevel::MediumStack:
        pthread_mutex_lock(&this->mutexMedium);
        i = this->DoneRoutinesMediumStacks.front();
        this->DoneRoutinesMediumStacks.pop_front();
        pthread_mutex_unlock(&this->mutexMedium);
        break;
    case StackLevel::LargeStack:
        pthread_mutex_lock(&this->mutexLarge);
        i = this->DoneRoutinesLargeStacks.front();
        this->DoneRoutinesLargeStacks.pop_front();
        pthread_mutex_unlock(&this->mutexLarge);
        break;
    default:
        break;
    }
    //   pthread_mutex_lock(&this->mutex);
    //   if (!this->DoneRoutinesStacks.empty()) {
    //     i = this->DoneRoutinesStacks.front();
    //     this->DoneRoutinesStacks.pop_front();
    //   }
    //   pthread_mutex_unlock(&this->mutex);
    return i;
}
void bRoutineEnv::bUnSharedStackPool_t::push(struct bRoutineStack* bst)
{
    //   pthread_mutex_lock(&this->mutex);
    //   this->DoneRoutinesStacks.emplace_back(bst);
    //   pthread_mutex_unlock(&this->mutex);
    switch (bst->StackSize / 1024) {
    case bRoutineStack::LargeStackSize:
        pthread_mutex_lock(&this->mutexLarge);
        this->DoneRoutinesMiniStacks.push_back(bst);
        pthread_mutex_unlock(&this->mutexLarge);
        break;
    case bRoutineStack::MediumStackSize:
        pthread_mutex_lock(&this->mutexMedium);
        this->DoneRoutinesMediumStacks.push_back(bst);
        pthread_mutex_unlock(&this->mutexMedium);
        break;
    case bRoutineStack::MiniStackSize:
        pthread_mutex_lock(&this->mutexMini);
        this->DoneRoutinesMiniStacks.push_back(bst);
        pthread_mutex_unlock(&this->mutexMini);
        break;
    default:
        break;
    }
}

static bRoutineEnv* __bRoutineEnv = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
#define defaultProcess 1

struct bRoutineEnv* InitializeBRoutineEnv(int RunProcessNums)
{
    if (__bRoutineEnv)
        return __bRoutineEnv;
    __bRoutineEnv = (bRoutineEnv*)calloc(1, sizeof(struct bRoutineEnv));
    __bRoutineEnv->TaskDistributor = bRoutineEnv::TaskDistributor_t::New(RunProcessNums);
    __bRoutineEnv->TaskDistributor->size = RunProcessNums;
    // __bRoutineEnv->ActiveTasks = TaskItemsList::New();
    __bRoutineEnv->RoutinePool = bRoutineEnv::bRoutinePool_t::New();
    __bRoutineEnv->UnSharedStackPool = bRoutineEnv::bUnSharedStackPool_t::New();
    __bRoutineEnv->Epoll = bEpoll::New();
    //   __bRoutineEnv->schedulers = (decltype(__bRoutineEnv->schedulers))calloc(
    //   __bRoutineEnv->runingProgressNum, sizeof(bScheduler *));

    return __bRoutineEnv;
}

void* ProcessBegin(void* i);

void CreatMutProcessEnv()
{
    bScheduler* thisSch = bScheduler::get();
    __bRoutineEnv->TaskDistributor->schedulers[0] = thisSch;
    // 挂起主线程
    auto threadArg = (threadArgs*)calloc(1, sizeof(threadArgs));
    bRoutine* mainR = bRoutine::New(StackLevel::LargeStack, ProcessBegin, threadArg);
    thisSch->pendingRoutine = mainR;
    thisSch->pendingRoutine->IsProgress = true;
    thisSch->occupyRoutine->IsMain = true;
    thisSch->SwapContext();
    // 切换上下文
    for (int i = 1; i < __bRoutineEnv->TaskDistributor->size; ++i) {
        auto threadArg = (threadArgs*)calloc(1, sizeof(threadArgs));
        pthread_create(&threadArg->thread, NULL, ProcessBegin, threadArg);
        // 起物理线程
        // 初始化物理线程的调度者
    }
}
struct bRoutineEnv* bRoutineEnv::init(uint RunProcessNums)
{
    if (__bRoutineEnv == NULL) {
        pthread_mutex_lock(&lock);
        if (__bRoutineEnv == NULL) {
            InitializeBRoutineEnv(RunProcessNums);
            CreatMutProcessEnv();
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
    int i = epoll_wait(this->Epoll->epollFd, this->Epoll->Revents->eventArr, this->Epoll->Revents->eventSize, 0);
    if (i == -1) {
        throw("epoll return false Erron :>" + std::to_string(errno));
    } else {
        auto& arr = this->Epoll->Revents->eventArr;
        TaskItem* task;
        for (int q = 0; q < i; q++) {
            task = (TaskItem*)arr[q].data.ptr;
            if (task->perpareFuncPtr)
                task->perpareFuncPtr(task->perpareFuncArgs);
            if (task->callbackFuncPtr)
                task->callbackFuncPtr(task->callbackFunArgs);
            epoll_ctl(this->Epoll->epollFd, EPOLL_CTL_DEL, task->epollFd, &task->bEpollEvent);
            this->TaskDistributor->AddTask(task);
        }
        task = nullptr;
        auto timeoutTaskList = this->Epoll->TimeoutScaner->NewMergeUpdate2Now();
        while ((task = timeoutTaskList->popHead()) != nullptr) {
            this->TaskDistributor->AddTask(task);
        }
        TaskItemsList::Delete(timeoutTaskList);
    }
    this->Epoll->lastCheckTime = bRoutineEnv::Timer::getNow_Ms();
}
void bRoutineEnv::bRoutineRecycle(bRoutine*& rt)
{
    this->UnSharedStackPool->push(rt->stack);
    rt->stack = nullptr;
    rt->context->stackPtr = nullptr;
    rt->context->stackSize = 0;

    this->RoutinePool->push(rt);
    rt = nullptr;
}
unsigned long long bRoutineEnv::Timer::getNow_S()
{
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
unsigned long long bRoutineEnv::Timer::getNow_Ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
unsigned long long bRoutineEnv::Timer::getNow_Us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
unsigned long long bRoutineEnv::Timer::getNow_Ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

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

        pthread_mutex_unlock(activeTask->TailMutex);
    } else {
        // 执行任务弹出操作
        a = activeTask->popHeadUnsafe();
    }
    pthread_mutex_unlock(activeTask->HeadMutex);
    return a;
}

struct TaskItem* bRoutineEnv::stealTaskRoutine(bScheduler* sch)
{
    auto i = this->TaskDistributor;
    TaskItem* a;
    for (int id = (sch->ID + 1) % i->size; id != sch->ID; ++id) {
        auto p = Try2stealTask(i->ActiveTasks[id]);
        if (p)
            sch->ActivetaskItems->pushBack(p);
    }
}
