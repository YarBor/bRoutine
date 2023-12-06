#include "../src/bRoutine.h"

void* Routine(void* arg)
{
    return 0;
}

int main()
{
    auto Rou = bRoutine::New(StackLevel::MediumStack, Routine, nullptr);
    Rou->Resume();
}