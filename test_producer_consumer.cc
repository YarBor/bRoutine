#include "bMutex_Cond.h"
#include "bRoutine.h"
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>
using namespace std;
struct stTask_t {
    int id;
};
struct stEnv_t {
    bCond* cond;
    queue<stTask_t*> task_queue;
};
void* Producer(void* args)
{
    stEnv_t* env = (stEnv_t*)args;
    int id = 0;
    while (true) {
        {
            stTask_t* task = (stTask_t*)calloc(1, sizeof(stTask_t));
            task->id = id++;
            env->task_queue.push(task);
            printf("%s:%d produce task %d\n", __func__, __LINE__, task->id);
        }
        {
            stTask_t* task = (stTask_t*)calloc(1, sizeof(stTask_t));
            task->id = id++;
            env->task_queue.push(task);
            printf("%s:%d produce task %d\n", __func__, __LINE__, task->id);
        }
        {
            stTask_t* task = (stTask_t*)calloc(1, sizeof(stTask_t));
            task->id = id++;
            env->task_queue.push(task);
            printf("%s:%d produce task %d\n", __func__, __LINE__, task->id);
        }
        {
            stTask_t* task = (stTask_t*)calloc(1, sizeof(stTask_t));
            task->id = id++;
            env->task_queue.push(task);
            printf("%s:%d produce task %d\n", __func__, __LINE__, task->id);
        }
        env->cond->signalAll();
        poll(NULL, 0, 1000);
    }
    return NULL;
}
void* Consumer(void* args)
{
    bMutex b;
    stEnv_t* env = (stEnv_t*)args;
    b.lock();
    while (true) {
        if (env->task_queue.empty()) {
            env->cond->timeWait(&b, 10000);
            continue;
        }
        stTask_t* task = env->task_queue.front();
        env->task_queue.pop();
        printf("%s:%d consume task %d\n", __func__, __LINE__, task->id);
        free(task);
    }
    return NULL;
}
int main()
{
    stEnv_t* env = new stEnv_t;
    env->cond = new bCond;

    bRoutine::New(StackLevel::MiniStack, Consumer, env)->Resume();
    bRoutine::New(StackLevel::MiniStack, Consumer, env)->Resume();
    bRoutine::New(StackLevel::MiniStack, Consumer, env)->Resume();

    bRoutine::New(StackLevel::MediumStack, Producer, env)->Resume();

    bRoutine::deleteSelf();
    // co_eventloop(co_get_epoll_ct(), NULL, NULL);
    return 0;
}