
.globl bSwap
#if !defined(__APPLE__)
    .type bSwap, @function
#endif

bSwap:
#if defined(__x86_64__)
    leaq    (%rsp), %rax         # 将栈指针的地址加载到 RAX 寄存器
    movq    %rax, 104(%rdi)      # 保存 RSP 的值到 RDI 指向的结构体中的偏移为 104 的位置
    movq    %rbx, 96(%rdi)       # 保存 RBX 的值到 RDI 指向的结构体中的偏移为 96 的位置
    movq    %rcx, 88(%rdi)       # 保存 RCX 的值到 RDI 指向的结构体中的偏移为 88 的位置
    movq    %rdx, 80(%rdi)       # 保存 RDX 的值到 RDI 指向的结构体中的偏移为 80 的位置
    movq    0(%rax), %rax        # 读取栈顶的值到 RAX，通常用于获取返回地址
    movq    %rax, 72(%rdi)       # 保存刚刚从栈顶获取的值到 RDI 指向的结构体中的偏移为 72 的位置
    movq    %rsi, 64(%rdi)       # 保存 RSI 的值到 RDI 指向的结构体中的偏移为 64 的位置
    movq    %rdi, 56(%rdi)       # 保存 RDI 的当前值到 RDI 指向的结构体中的偏移为 56 的位置
    movq    %rbp, 48(%rdi)       # 保存 RBP 的值到 RDI 指向的结构体中的偏移为 48 的位置
    movq    %r8, 40(%rdi)        # 保存 R8 的值到 RDI 指向的结构体中的偏移为 40 的位置
    movq    %r9, 32(%rdi)        # 保存 R9 的值到 RDI 指向的结构体中的偏移为 32 的位置
    movq    %r12, 24(%rdi)       # 保存 R12 的值到 RDI 指向的结构体中的偏移为 24 的位置
    movq    %r13, 16(%rdi)       # 保存 R13 的值到 RDI 指向的结构体中的偏移为 16 的位置
    movq    %r14, 8(%rdi)        # 保存 R14 的值到 RDI 指向的结构体中的偏移为 8 的位置
    movq    %r15, (%rdi)         # 保存 R15 的值到 RDI 指向的结构体的起始位置
    xorq    %rax, %rax           # 清零 RAX 寄存器

    movq    48(%rsi), %rbp       # 恢复 RBP 寄存器，将 RSI 指向的结构体中的偏移为 48 的值加载到 RBP
    movq    104(%rsi), %rsp      # 恢复 RSP 寄存器，将 RSI 指向的结构体中的偏移为 104 的值加载到 RSP
    movq    (%rsi), %r15         # 恢复 R15 寄存器
    movq    8(%rsi), %r14        # 恢复 R14 寄存器
    movq    16(%rsi), %r13       # 恢复 R13 寄存器
    movq    24(%rsi), %r12       # 恢复 R12 寄存器
    movq    32(%rsi), %r9        # 恢复 R9 寄存器
    movq    40(%rsi), %r8        # 恢复 R8 寄存器
    movq    56(%rsi), %rdi       # 恢复 RDI 寄存器
    movq    80(%rsi), %rdx       # 恢复 RDX 寄存器
    movq    88(%rsi), %rcx       # 恢复 RCX 寄存器
    movq    96(%rsi), %rbx       # 恢复 RBX 寄存器
    leaq    8(%rsp), %rsp        # 调整栈指针，跳过之前保存的返回地址
    pushq   72(%rsi)             # 将返回地址压栈，准备通过 ret 指令返回

    movq    64(%rsi), %rsi       # 恢复 RSI 寄存器
    ret                           # 从之前保存的返回地址返回
#endif
