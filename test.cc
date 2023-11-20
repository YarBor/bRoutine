#include "bRoutine.h"
#include <iostream>
#include <string>
#include <vector>
void* print2(void* data);
int asdf = 10;
void* print1(void* data)
{
    printf("1\n");
    // return 0;
    if (asdf-- != 0) {
        auto i = bRoutine::New(StackLevel::MediumStack, print1, NULL);
        i->Resume();
    }
    return 0;
}
void* print2(void* data)
{
    printf("2\n");
    bRoutine::yield();
    return 0;
}
void* print3(void* data)
{
    printf("3\n");
    bRoutine::yield();
    return 0;
}
void* print4(void* data)
{
    printf("4\n");
    bRoutine::yield();
    return 0;
}

int main()
{
    // read(1, nullptr, 0);
    bRoutineInitProcessNumber(1);
    {
        auto i = bRoutine::New(StackLevel::MediumStack, print1, NULL);
        i->Resume();
    }
    {
        auto i = bRoutine::New(StackLevel::MediumStack, print2, NULL);
        i->Resume();
    }
    {
        auto i = bRoutine::New(StackLevel::MediumStack, print3, NULL);
        i->Resume();
    }
    {
        auto i = bRoutine::New(StackLevel::MediumStack, print4, NULL);
        i->Resume();
    }
}