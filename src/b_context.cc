#include "b_context.h"

#include "b_scheduler.h"
void contextSwap(bRoutine*, bRoutine*);
bContext* bContext::New()
{
    bContext* ret = (bContext*)calloc(1, sizeof(bContext));
    return ret;
}
bContext* bContext::init(bRoutine* self, bRoutineStack* stack)
{
    this->stackSize = stack->StackSize;
    this->stackPtr = (char*)stack->StackPtr;
    char* sp = this->stackPtr + bRoutineStack::GetPageSize_b() + this->stackSize - 1;
    sp = (char*)((unsigned long)sp & -16LL);
    this->regs[kRSP] = sp;
    this->regs[kRDI] = self;
    this->regs[kRSI] = NULL;
    this->regs[kRETAddr] = (char*)RoutineBegin;
    return this;
}

// not free Stack pointer
void bContext::Del(bContext* i)
{
    free(i);
}

// int bContext::initContext(RoutineFunc *func, void *args) { return 0; }