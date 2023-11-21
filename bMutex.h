#pragma once
#include "bRoutine.h"
#include <atomic>
#include <pthread.h>
struct bMutexCondTask {
    struct bMutexCondTask* prev = NULL;
    struct bMutexCondTask* next = NULL;
    bRoutine* routine = NULL;
};
struct bMutexCondTaskList {
    struct bMutexCondTask* head = NULL;
    struct bMutexCondTask* tail = NULL;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    bool add(bRoutine*);
    bMutexCondTask* pop();
};
struct bMutex {
    std::atomic<int> lockCount;
    bMutexCondTaskList* list;
    void lock();
    void unlock();
    bMutex()
    {
        this->list = new bMutexCondTaskList;
    }
    ~bMutex()
    {
        delete list;
    }
};
struct bCond {
    bMutexCondTaskList* list;
    void wait(bMutex *);
    int timeWait(bMutex *,int timeout_ms);
    void signalOnce();
    void signalAll();
    bCond()
    {
        this->list = new bMutexCondTaskList;
    }
    ~bCond()
    {
        delete list;
    }
};