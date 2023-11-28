#include "common.h"
#include "bRoutine.h"
#include "bRoutineEnv.h"
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>

void* Routine(void* arg)
{
    return 0;
}

int main()
{
    auto Rou = bRoutine::New(StackLevel::MediumStack, Routine, nullptr);
    Rou->Resume();
}