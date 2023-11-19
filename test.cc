#include "bRoutine.h"
#include <iostream>
#include <string>
#include <vector>
void* print2(void* data);
void* print1(void* data)
{
    printf("1\n");
    // return 0;
    auto i = bRoutine::New(StackLevel::MediumStack, print2, NULL);
    i->Resume();
    return 0;
}
void* print2(void* data)
{
    printf("2\n");
    return 0;
}
void* print3(void* data)
{
    printf("3\n");
    return 0;
}
void* print4(void* data)
{
    printf("4\n");
    return 0;
}

int main()
{
    read(1, nullptr, 0);
}