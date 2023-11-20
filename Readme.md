## bRoutine

基于Linux x86_64 下的 N:M 的协程库

使用非抢占式调度 
Hook了常用的 网络IO系统调用 基于epoll 进行事件触发,上下文切换

上下文切换是手搓的汇编
用汇编保存当前协程的寄存器状态

是对称协程库 协程栈为协程私有 

单Epoll实例 
每个物理线程维护一个就绪任务队列
当物理线程私有就绪队列为空时 可以窃取其他线程的就绪任务

超时事件采用单级时间轮, 以1ms为最小超时时间单位 ,最长超时时间通过`common.h`定义