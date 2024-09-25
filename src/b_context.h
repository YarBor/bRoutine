#pragma once
#include <stdlib.h>

#include "b_routine.h"
#include "b_stack.h"
#define RSP 0
#define RIP 1
#define RBX 2
#define RDI 3
#define RSI 4

#define RBP 5
#define R12 6
#define R13 7
#define R14 8
#define R15 9
#define RDX 10
#define RCX 11
#define R8 12
#define R9 13

//-------------
// 64 bit
// low | regs[0]: r15 |
//    | regs[1]: r14 |
//    | regs[2]: r13 |
//    | regs[3]: r12 |
//    | regs[4]: r9  |
//    | regs[5]: r8  |
//    | regs[6]: rbp |
//    | regs[7]: rdi |
//    | regs[8]: rsi |
//    | regs[9]: ret |  //ret func addr
//    | regs[10]: rdx |
//    | regs[11]: rcx |
//    | regs[12]: rbx |
// hig | regs[13]: rsp |
enum {
    kRDI = 7,
    kRSI = 8,
    kRETAddr = 9,
    kRSP = 13,
};
typedef void*(RoutineFunc)(void*);

struct bContext {
    // 寄存器的存放位置
    void* regs[14];
    // 栈的大小
    size_t stackSize;
    // 栈的起始地址
    char* stackPtr;
    static  bContext* New();
    bContext* init(struct bRoutine*, bRoutineStack*);
    static  void Del(bContext*);
    //   int initContext(RoutineFunc *func, void *arg);
}; // 128

#if defined(__x86_64__) || defined(_M_X64)
extern "C" {
extern void bSwap(bContext*, bContext*) asm("bSwap");
};
#else
#error "Unsupported architecture. Only x86-64 is supported."
#endif
