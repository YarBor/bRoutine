#include "../src/bMutex_Cond.h"
#include "../src/bRoutine.h"
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>
using namespace std;
struct Task {
    int id;
};
struct TaskQueue {
    bCond* cond;
    queue<Task*> task_queue;
};
void* Producer(void* args)
{
    TaskQueue* env = (TaskQueue*)args;
    int id = 0;
    while (true) {
        for (int i = 0, l = rand() % 10; i < l; i++) {
            Task* task = (Task*)calloc(1, sizeof(Task));
            task->id = id++;
            env->task_queue.push(task);
            printf("%s:%d produce task %d\n", __func__, __LINE__, task->id);
        }
        env->cond->signalAll();
        pollfd i = { 0, POLLIN, 0 };
        poll(&i, 1, 1000);
    }
    return NULL;
}
void* Consumer(void* args)
{
    static int ConsumerId = 0;
    int Id = ++ConsumerId;
    bMutex b;
    TaskQueue* env = (TaskQueue*)args;
    b.lock();
    while (true) {
        if (env->task_queue.empty()) {
            env->cond->timeWait(&b, 10000);
            continue;
        }
        Task* task = env->task_queue.front();
        env->task_queue.pop();
        printf("%s:%d consume task %d\n", __func__, Id, task->id);
        free(task);
    }
    return NULL;
}
int main()
{
    TaskQueue* env = new TaskQueue;
    env->cond = new bCond;
    bRoutine::New(StackLevel::MiniStack, Consumer, env)->Resume();
    bRoutine::New(StackLevel::MediumStack, Producer, env)->Resume();
    bRoutine::deleteSelf();
    return 0;
}