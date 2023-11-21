#include "bRoutine.h"
#include <iostream>
#include <string>
#include <sys/poll.h>
#include <vector>
// void* print2(void* data);
int asdf = 10;
int iasd = 0;
void* print1(void* data)
{
    poll(NULL, 0, 100);
    printf("1\n");
    iasd = 1;
    return 0;
}

int main()
{
    bRoutineInitProcessNumber(2);
    auto i = bRoutine::New(StackLevel::MiniStack, print1, NULL);
    i->Resume();
    while (!iasd)
        bRoutine::yield();
    poll(NULL, 0, 1000);
    printf("2\n");
}