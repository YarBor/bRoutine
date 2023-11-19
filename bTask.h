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
typedef void*(PerpareFuncPtr)(void*);
typedef void*(CallBackFuncPtr)(void*);

struct TaskItemsList;
struct TaskItem {
    TaskItem* prev;
    TaskItem* next;
    TaskItemsList* owner;
    PerpareFuncPtr* perpareFuncPtr;
    void* perpareFuncArgs;
    CallBackFuncPtr* callbackFuncPtr;
    void* callbackFunArgs;
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
