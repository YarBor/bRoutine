#pragma once
#include <pthread.h>

#include <list>

#include "bEpoll.h"
#include "bRoutine.h"
#include "bScheduler.h"
#include "bStack.h"
struct bRoutineEnv {
    bRoutineEnv() = delete;
    struct bRoutinePool_t {
        pthread_mutex_t mutex;
        std::list<struct bRoutine*> DoneRoutines;

        struct bRoutine* pop();
        void push(struct bRoutine*);
        static struct bRoutinePool_t* New();
    };
    struct bUnSharedStackPool_t {
        pthread_mutex_t mutexMini;
        pthread_mutex_t mutexMedium;
        pthread_mutex_t mutexLarge;
        std::list<struct bRoutineStack*> DoneRoutinesMiniStacks;
        std::list<struct bRoutineStack*> DoneRoutinesMediumStacks;
        std::list<struct bRoutineStack*> DoneRoutinesLargeStacks;

        struct bRoutineStack* pop(int Level);
        void push(struct bRoutineStack*);
        static struct bUnSharedStackPool_t* New();
    };
    struct Timer {
        static inline unsigned long long getNow_S();
        static inline unsigned long long getNow_Ms();
        static inline unsigned long long getNow_Us();
        static inline unsigned long long getNow_Ns();
    };
    struct TaskDistributor_t {
        bScheduler** schedulers;
        struct TaskItemsList** ActiveTasks;
        int size;
        int registe(bScheduler*);
        static TaskDistributor_t* New(int);
        void AddTask(struct TaskItem*);
    };
    struct bRoutinePool_t* RoutinePool;
    struct bUnSharedStackPool_t* UnSharedStackPool;
    struct TaskDistributor_t* TaskDistributor;
    pthread_mutex_t bEpollDealLock;
    struct bEpoll* Epoll;
    static struct bRoutineEnv* init(uint runingProgressNum);
    static struct bRoutineEnv* get();
    void bRoutineRecycle(bRoutine*&);
    struct TaskItem* stealTaskRoutine(bScheduler*);
    // 外部保证原子
    void Deal();
};
