#pragma once
#include "b_routine.h"
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
    pthread_mutex_t mutex;
    bool add(bRoutine*);
    bMutexCondTask* pop();
};
struct bMutex {
    std::atomic<int> lockCount;
    bMutexCondTaskList* list;
    int lock();
    int unlock();
    bMutex()
    {
        this->list = new bMutexCondTaskList;
        this->list->mutex = PTHREAD_MUTEX_INITIALIZER;
    }
    ~bMutex()
    {
        pthread_mutex_destroy(&this->list->mutex);
        delete list;
    }
};
struct bCond {
    bMutexCondTaskList* list;
    void wait(bMutex*);
    int timeWait(bMutex*, int timeout_ms);
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