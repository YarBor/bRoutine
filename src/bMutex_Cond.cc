#include "bMutex_Cond.h"
#include "bRoutineEnv.h"
bool bMutexCondTaskList::add(bRoutine* r)
{
retry:
    if (!pthread_mutex_trylock(&mutex)) {
        DebugPrint("%d bMutexCond List lock successed\n", r->id);
        auto i = new bMutexCondTask;
        i->routine = r;
        if (tail == NULL) {
            head = i;
            tail = i;
        } else {
            tail->next = i;
            i->prev = tail;
            tail = i;
        }
        DebugPrint("%d bMutexCond List lock relased\n", r->id);
        pthread_mutex_unlock(&mutex);
    } else {
        DebugPrint("%d bMutexCond List lock failed\n", r->id);
        bRoutine::getSelf()->yield();
        goto retry;
    }
    return true;
}
bMutexCondTask* bMutexCondTaskList::pop()
{
    bMutexCondTask* result;
retry:
    if (!pthread_mutex_trylock(&this->mutex)) {
        DebugPrint("%d bMutexCond List lock successed\n", bRoutine::getSelf()->id);
        if (head == NULL)
            result = 0;
        else if (head == tail) {
            result = head;
            head = NULL;
            tail = NULL;
        } else {
            result = head;
            head = head->next;
            head->prev = NULL;
            result->next = NULL;
        }
        DebugPrint("%d bMutexCond List lock relased\n", bRoutine::getSelf()->id);
        pthread_mutex_unlock(&this->mutex);
    } else {
        DebugPrint("%d bMutexCond List lock failed\n", bRoutine::getSelf()->id);
        bRoutine::getSelf()->yield();
        goto retry;
    }
    return result;
}

int bMutex::lock()
{
    int i = 0;
    bool isSuccessLock = false;
retry:
    DebugPrint("%d try Locked\n", bRoutine::getSelf()->id);
    isSuccessLock = this->lockCount.compare_exchange_strong(i, 1);
    if (!isSuccessLock) {
        DebugPrint("%d try Locke false\n", bRoutine::getSelf()->id);
        if (this->list->add(bRoutine::getSelf())) {
            DebugPrint("%d add self to lock List\n", bRoutine::getSelf()->id);
            bScheduler::get()->SwapContext();
        } else {
            DebugPrint("%d yield self\n", bRoutine::getSelf()->id);
            bRoutine::getSelf()->yield();
        }
        goto retry;
    }
    DebugPrint("%d Locked\n", bRoutine::getSelf()->id);
    return isSuccessLock;
}
int bMutex::unlock()
{
    int i = 1;
    bool isSuccessUnLock;
    isSuccessUnLock = this->lockCount.compare_exchange_strong(i, 0);
    if (!isSuccessUnLock) {
        abort();
    }
    DebugPrint("%d unLocked\n", bRoutine::getSelf()->id);
    auto mt = this->list->pop();
    if (mt) {
        auto t = TaskItem::New();
        t->self = mt->routine;
        delete mt;
        bRoutineEnv::get()->TaskDistributor->AddTask(t);
    }
    return isSuccessUnLock;
}
void bCond::wait(bMutex* lock)
{
    lock->unlock();
    this->list->add(bRoutine::getSelf());
    bScheduler::get()->SwapContext();
    lock->lock();
}
void* CondTimeOutCallback(void* args)
{
    int* i = (int*)args;
    *i = 1;
    return 0;
}
int bCond::timeWait(bMutex* lock, int timeout_ms)
{
    lock->unlock();
    this->list->add(bRoutine::getSelf());
    auto t = TaskItem::New();
    t->timeoutCallFunc = CondTimeOutCallback;
    int IsTimeout = 0;
    t->timeoutCallFuncArgs = &IsTimeout;
    t->self = bRoutine::getSelf();
    bRoutineEnv::get()->Epoll->TimeoutScaner->getBucket(timeout_ms)->pushBack(t);
    bScheduler::get()->SwapContext();
    if (IsTimeout == 0) {
        RemoveSelfFromOwnerlink_Save(t);
        lock->lock();
        return 1;
    } else {
        return 0;
    }
}
void bCond::signalAll()
{
    bMutexCondTask* t = 0;
    bRoutineEnv* env = bRoutineEnv::get();
    while ((t = this->list->pop()) != NULL) {
        auto ta = TaskItem::New();
        ta->self = t->routine;
        env->TaskDistributor->AddTask(ta);
        delete t;
    }
}
void bCond::signalOnce()
{
    bRoutineEnv* env = bRoutineEnv::get();
    bMutexCondTask* t = this->list->pop();
    auto ta = TaskItem::New();
    ta->self = t->routine;
    env->TaskDistributor->AddTask(ta);
    delete t;
}