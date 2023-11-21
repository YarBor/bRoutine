#include "bMutex.h"
#include "bRoutineEnv.h"
bool bMutexCondTaskList::add(bRoutine* r)
{
    if (pthread_mutex_trylock(&this->mutex)) {
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
        pthread_mutex_unlock(&this->mutex);
        return true;
    } else {
        return false;
    }
}
bMutexCondTask* bMutexCondTaskList::pop()
{
    bMutexCondTask* result;
    pthread_mutex_lock(&this->mutex);
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
    pthread_mutex_unlock(&this->mutex);
    return result;
}

void bMutex::lock()
{
    int i = 0;
    bool isSuccessLock;
retry:
    isSuccessLock = this->lockCount.compare_exchange_strong(i, 1);
    if (!isSuccessLock) {
        if (this->list->add(bRoutine::getSelf())) {
            bScheduler::get()->SwapContext();
        }
        goto retry;
    }
}
void bMutex::unlock()
{
    int i = 1;
    bool isSuccessLock;
    isSuccessLock = this->lockCount.compare_exchange_strong(i, 0);
    if (!isSuccessLock) {
        abort();
    }
    auto mt = this->list->pop();
    if (mt) {
        auto t = TaskItem::New();
        t->self = mt->routine;
        delete mt;
        bRoutineEnv::get()->TaskDistributor->AddTask(t);
    }
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
    bMutexCondTask* t = 0;
    bRoutineEnv* env = bRoutineEnv::get();
    auto ta = TaskItem::New();
    ta->self = t->routine;
    env->TaskDistributor->AddTask(ta);
    delete t;
}