#define _TEST_SWAP_TIEM
#include "bRoutine.h"
#include "bRoutineEnv.h"
#include <queue>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>
using namespace std;
void* Routine(void* arg)
{
    while (1) {
        poll(nullptr, 0, 1000);
    }
}
int main()
{
    int i;
    int count = 100000;
    try {
        bRoutineInitProcessNumber(1);
        printf("%d\n", getpid());
        auto i1 = bRoutineEnv::Timer::getNow_Ms();
        for (i = 0; i < count; i++) {
            bRoutine::New(StackLevel::MiniStack, Routine, NULL)->Resume();
        }
        auto i2 = bRoutineEnv::Timer::getNow_Ms();
        printf("Create %d bRoutines , using %lld ms\n", count, i2 - i1);
        poll(nullptr, 0, 1500);
        return 0;
    } catch (char const* error) {
        printf("created %d bRoutines", i);
        perror(error);
        exit(1);
    }
}