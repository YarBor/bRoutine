#include "bRoutine.h"
#include <iostream>
#include <string>
#include <sys/poll.h>
#include <vector>
// void* print2(void* data);
int asdf = 10;
void* print1(void* data)
{
    while (1) {
        poll(NULL, 0, 100);
        printf("1\n");
    }
    return 0;
}

int main()
{
    bRoutineInitProcessNumber(1);
    auto i = bRoutine::New(StackLevel::MiniStack, print1, NULL);
    i->Resume();
    while (1) {
        poll(NULL, 0, 1000);
        printf("2\n");
    }
}