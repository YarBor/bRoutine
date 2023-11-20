#define Max_Keep_Routines_Thread 1000
// 默认线程数量
#define defaultProcess 2
// hook最多监控多少文件描述符
#define HookContralFdNums 102400
#define HookContralFdTimeout_S 1
#define HookContralFdTimeout_MS 0
// 协程栈 不同等级的大小
#define MiniStackSize__ 128
#define MediumStackSize__ 1024
#define LargeStackSize__ 4096
// 执行线程 无事 挂起时间
#define SchedulerHangTimeout_S 0
#define SchedulerHangTimeout_Ms 0.5
#define SchedulerHangTimeout_Ns 0
// epoll 时间轮 监控了多长的时间
#define TIMEWAITSECONDS_S 30
// epoll 同一时间返回的事件容量
#define BEpollMaxEventsSize 1e5
