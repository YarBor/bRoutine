// #include <pthread.h>
// #include <stdlib.h>
// #include <string.h>

#include "bRoutineEnv.h"

bRoutine* bRoutine::Alloc()
{
    auto i = (bRoutine*)calloc(1, sizeof(bRoutine));
    return i;
}
bRoutine* bRoutine::New(int StackLevel, RoutineFunc* func, void* args)
{
    auto Env = bRoutineEnv ::get();
    bRoutine* i;
    if ((i = Env->RoutinePool->pop()) == nullptr) {
        i = (bRoutine*)calloc(1, sizeof(bRoutine));
        i->context = bContext::New();
    }
    if ((i->stack = Env->UnSharedStackPool->pop(StackLevel)) == nullptr) {
        i->stack = bRoutineStack::New(StackLevel);
    }
    i->context->init(i,i->stack);
    i->func = func;
    i->args = args;
    // i->context->initContext(RoutineBegin, NULL);
    return i;
}
// 把目标加入就绪队列 并把自己切下去
void bRoutine::Resume()
{
    auto Env = bRoutineEnv::get();
    auto Sch = bScheduler::get();
    auto taskItem1 = TaskItem::New();
    auto taskItem2 = TaskItem::New();
    taskItem1->self = this;
    taskItem2->self = Sch->occupyRoutine;
    Env->TaskDistributor->AddTask(taskItem1);
    Env->TaskDistributor->AddTask(taskItem2);
    Sch->SwapContext();
}
void bRoutine::yield()
{
    auto Env = bRoutineEnv::get();
    auto Sch = bScheduler::get();
    auto taskItem2 = TaskItem::New();
    taskItem2->self = Sch->occupyRoutine;
    Env->TaskDistributor->AddTask(taskItem2);
    Sch->SwapContext();
}