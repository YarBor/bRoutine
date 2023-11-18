#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "bRoutineEnv.h"

template <typename T>
T inline* RemoveSelfFromOwnerlink(T* i)
{
    if (i == nullptr || i->owner == nullptr || i == i->owner->head)
        return nullptr; // 如果i为空，则直接返回nullptr。
                        //   assert(i != i->owner->head);
    if (i == i->owner->tail) {
        i->owner->tail = i->prev;
        i->prev->next = nullptr;
        i->prev = nullptr;
        i->owner = nullptr;
        return i;
    }
    auto p = i->prev;
    auto n = i->next;
    i->owner = nullptr;
    i->prev->next = n;
    i->next->prev = p;
    i->next = nullptr;
    i->prev = nullptr;
    return i;
}

struct TaskItem* TaskItem::New()
{
    struct TaskItem* i = (struct TaskItem*)calloc(1, sizeof(struct TaskItem));
    memset(i, 0, sizeof(struct TaskItem));
    i->next = NULL;
    i->prev = NULL;
    return i;
}
void TaskItem::Delete(TaskItem*& i)
{
    free(i);
    i = NULL;
}
struct TaskItemsList* TaskItemsList::New()
{
    struct TaskItemsList* i = (struct TaskItemsList*)calloc(1, sizeof(struct TaskItemsList));
    i->HeadMutex = (decltype(i->HeadMutex))calloc(1, sizeof(pthread_mutex_t));
    *i->HeadMutex = PTHREAD_MUTEX_INITIALIZER;
    i->TailMutex = (decltype(i->TailMutex))calloc(1, sizeof(pthread_mutex_t));
    *i->TailMutex = PTHREAD_MUTEX_INITIALIZER;
    i->head = TaskItem::New();
    i->head->owner = i;
    i->tail = i->head;
    i->ItemCount = 0;
    return i;
}

void TaskItemsList::pushBack(TaskItem* item)
{
    bool lockHead = false;
    if (this->ItemCount.load() < 2) {
        pthread_mutex_lock(this->HeadMutex);
        lockHead = true;
    }

    pthread_mutex_lock(this->TailMutex);

    this->pushBackUnsafe(item);

    if (lockHead) {
        pthread_mutex_unlock(this->HeadMutex);
    }

    pthread_mutex_unlock(this->TailMutex);
}

TaskItem* TaskItemsList::popHead()
{
    if (this->ItemCount.load() == 0) {
        return nullptr;
    }

    bool lockTail = false;

    pthread_mutex_lock(this->HeadMutex);

    if (this->ItemCount.load() < 2) {
        pthread_mutex_lock(this->TailMutex);
        lockTail = true;
    }

    TaskItem* item = this->popHeadUnsafe();

    pthread_mutex_unlock(this->HeadMutex);

    if (lockTail) {
        pthread_mutex_unlock(this->TailMutex);
    }

    return item;
}

void TaskItemsList::pushBackUnsafe(TaskItem* i)
{
    this->tail->next = i;
    i->prev = this->tail;
    this->tail = i;
    i->next = nullptr;
    i->owner = this;
    ++this->ItemCount;
}
TaskItem* TaskItemsList::popHeadUnsafe()
{
    if (this->ItemCount == 0)
        return nullptr;
    auto i = RemoveSelfFromOwnerlink(this->head->next);
    --ItemCount;
    return i;
}

// safe
void TaskItemsList::pushAll2(TaskItemsList* i)
{
    MtxLockGuard(this->HeadMutex);
    MtxLockGuard(this->TailMutex);
    MtxLockGuard(i->TailMutex);
    MtxLockGuard(i->TailMutex);
    TaskItem* t;
    while ((t = RemoveSelfFromOwnerlink(this->tail)) != nullptr) {
        --this->ItemCount;
        i->pushBackUnsafe(t);
        ++i->ItemCount;
    }
}
void TaskItemsList::Delete(TaskItemsList*& i)
{
    auto self = i;
    {
        MtxLockGuard(self->HeadMutex);
        MtxLockGuard(self->TailMutex);
        i = nullptr;
    }
    TaskItem* t;
    while ((t = RemoveSelfFromOwnerlink(self->tail)) != nullptr) {
        TaskItem::Delete(t);
    }
    TaskItem::Delete(self->head);
    free(self->TailMutex);
    free(self->HeadMutex);
    free(self);
}
bRoutine* bRoutine::Alloc()
{
    auto i = (bRoutine*)calloc(1, sizeof(bRoutine));
    return i;
}
bRoutine* bRoutine::New(int StackLevel, RoutineFunc* func, void* args)
{
    auto Env = bRoutineEnv ::get();
    bRoutine* i;
    if ((i = Env->RoutinePool->pop()) == nullptr) {
        i = (bRoutine*)calloc(1, sizeof(bRoutine));
        i->context = bContext::New();
    }
    if ((i->stack = Env->UnSharedStackPool->pop(StackLevel)) == nullptr) {
        i->stack = bRoutineStack::New(StackLevel);
    }
    i->context->init(i->stack);
    i->func = func;
    i->args = args;
    // i->context->initContext(RoutineBegin, NULL);
    return i;
}
// 把目标加入就绪队列 并把自己切下去
void bRoutine::Resume()
{
    auto Env = bRoutineEnv::get();
    auto Sch = bScheduler::get();
    auto taskItem1 = TaskItem::New();
    auto taskItem2 = TaskItem::New();
    taskItem1->self = this;
    taskItem2->self = Sch->occupyRoutine;
    Env->TaskDistributor->AddTask(taskItem1);
    Env->TaskDistributor->AddTask(taskItem2);
    Sch->SwapContext();
}
void bRoutine::yield()
{
    auto Env = bRoutineEnv::get();
    auto Sch = bScheduler::get();
    auto taskItem2 = TaskItem::New();
    taskItem2->self = Sch->occupyRoutine;
    Env->TaskDistributor->AddTask(taskItem2);
    Sch->SwapContext();
}