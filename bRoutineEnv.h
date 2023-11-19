#pragma once
#include <pthread.h>

// #include <list>

#include "bEpoll.h"
#include "bRoutine.h"
#include "bScheduler.h"
#include "bStack.h"
template <class T>
struct List {
    template <class T1>
    struct ListNode {
        ListNode<T1>* next = nullptr;
        ListNode<T1>* prev = nullptr;
        T1* value = nullptr;
    };
    ListNode<T>* Head = nullptr;
    ListNode<T>* Tail = nullptr;
    size_t _size = 0;
    size_t size() const { return _size; }
    T* pop()
    {
        if (Head == nullptr)
            return nullptr;
        _size--;
        if (Tail == Head) {
            auto i = Head;
            this->Head = nullptr;
            this->Tail = nullptr;
            auto ret = i->value;
            delete (i);
            return ret;
        }
        auto i = Head;
        Head = Head->next;
        Head->prev = nullptr;
        auto ret = i->value;
        delete (i);
        return ret;
    }
    void push(T* val)
    {
        _size++;
        auto i = new (ListNode<T>);
        if (Tail == nullptr) {
            Head = i;
            Tail = i;
            i->value = val;
        } else {
            Tail->next = i;
            i->prev = Tail;
            this->Tail = i;
            i->value = val;
        }
    }
};
struct bRoutineEnv {
    bRoutineEnv() = delete;
    struct bRoutinePool_t {
        pthread_mutex_t mutex;
        List<struct bRoutine> DoneRoutines;

        struct bRoutine* pop();
        void push(struct bRoutine*);
        static struct bRoutinePool_t* New();
    };
    struct bUnSharedStackPool_t {
        pthread_mutex_t mutexMini;
        pthread_mutex_t mutexMedium;
        pthread_mutex_t mutexLarge;
        List<struct bRoutineStack> DoneRoutinesMiniStacks;
        List<struct bRoutineStack> DoneRoutinesMediumStacks;
        List<struct bRoutineStack> DoneRoutinesLargeStacks;

        struct bRoutineStack* pop(int Level);
        void push(struct bRoutineStack*);
        static struct bUnSharedStackPool_t* New();
    };
    struct Timer {
        static unsigned long long getNow_S();
        static unsigned long long getNow_Ms();
        static unsigned long long getNow_Us();
        static unsigned long long getNow_Ns();
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
