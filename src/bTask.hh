#pragma once
#include "bEpoll.h"
#include <atomic>
// #include <pthread.h>

#define CON(X, Y) X##Y
#define MtxLockGuard_(X, NAME, LINE) LockGuard CON(NAME, LINE)(X)
#define MtxLockGuard(Ptr_pthread_mutex_lock) MtxLockGuard_(Ptr_pthread_mutex_lock, MtxLockGuardName_, __LINE__)

class LockGuard {
public:
    LockGuard(pthread_mutex_t* m)
        : m(m)
    {
        pthread_mutex_lock(this->m);
    };
    ~LockGuard()
    {
        pthread_mutex_unlock(this->m);
    };

private:
    pthread_mutex_t* m;
};
typedef void*(PerpareFuncPtr)(void*, void*);
typedef void*(CallBackFuncPtr)(void*);
typedef void*(TimeOutCallFuncPtr)(void*);

struct TaskItemsList;
struct TaskItem {
    TaskItem* prev;
    TaskItem* next;
    TaskItemsList* owner;
    // args + epoll_revents
    PerpareFuncPtr* perpareFuncPtr;
    void* perpareFuncArgs;
    CallBackFuncPtr* callbackFuncPtr;
    void* callbackFunArgs;
    TimeOutCallFuncPtr* timeoutCallFunc;
    void* timeoutCallFuncArgs;
    bRoutine* self;
    epoll_event bEpollEvent;
    int epollFd;
    bool IsTasktimeout;
    static struct TaskItem* New();
    static void Delete(TaskItem*&);
};
struct TaskItemsList {
    pthread_mutex_t* HeadMutex;
    pthread_mutex_t* TailMutex;
    TaskItem* head;
    TaskItem* tail;
    std::atomic_uint32_t ItemCount; // without head
    static struct TaskItemsList* New();
    static void Delete(TaskItemsList*&);
    TaskItem* popHead();
    void pushBack(TaskItem*);
    void pushAll2(TaskItemsList*);
    //   static struct TaskItemsList *newAndPushAll(TaskItemsList *,TaskItemsList
    //   *);

    void pushBackUnsafe(TaskItem* i);
    TaskItem* popHeadUnsafe();
};
template <typename T>
T* RemoveSelfFromOwnerlink(T* i)
{
    if (i == nullptr || i->owner == nullptr )
        return nullptr; // 如果i为空，则直接返回nullptr。
                        //   assert(i != i->owner->head);
    if (i->owner->head == i) {
        if (i == i->owner->tail) {
            i->owner->head = nullptr;
            i->owner->tail = nullptr;
        } else {
            i->next->prev = NULL;
            i->owner->head = i->next;
        }
        i->next = NULL;
        i->owner = NULL;
        return i;
    }
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
    p->next = n;
    n->prev = p;
    i->next = nullptr;
    i->prev = nullptr;
    return i;
}
template <typename T>
T* RemoveSelfFromOwnerlink_Save(T* i)
{
    // pthread_mutex_t* ;
    LockGuard(i->owner->HeadMutex);
    LockGuard(i->owner->TailMutex);
    auto a = RemoveSelfFromOwnerlink(i);
    return a;
}