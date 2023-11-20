#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <resolv.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "bRoutineEnv.h"
#include <map>

typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*accept_pfn_t)(int fd, struct sockaddr* addr, socklen_t* len);
typedef int (*connect_pfn_t)(int socket, const struct sockaddr* address, socklen_t address_len);
typedef int (*close_pfn_t)(int fd);
typedef ssize_t (*read_pfn_t)(int fildes, void* buf, size_t nbyte);
typedef ssize_t (*write_pfn_t)(int fildes, const void* buf, size_t nbyte);
typedef ssize_t (*sendto_pfn_t)(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr, socklen_t dest_len);
typedef ssize_t (*recvfrom_pfn_t)(int socket, void* buffer, size_t length, int flags, struct sockaddr* address, socklen_t* address_len);
typedef ssize_t (*send_pfn_t)(int socket, const void* buffer, size_t length, int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void* buffer, size_t length, int flags);
typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
typedef int (*setsockopt_pfn_t)(int socket, int level, int option_name, const void* option_value, socklen_t option_len);
typedef int (*fcntl_pfn_t)(int fildes, int cmd, ...);

#define Hook_Func(X) Hook_##X##_func
static read_pfn_t Hook_Func(read) = (read_pfn_t)dlsym(RTLD_NEXT, "read");
static write_pfn_t Hook_Func(write) = (write_pfn_t)dlsym(RTLD_NEXT, "write");
static close_pfn_t Hook_Func(close) = (close_pfn_t)dlsym(RTLD_NEXT, "close");
static socket_pfn_t Hook_Func(socket) = (socket_pfn_t)dlsym(RTLD_NEXT, "socket");
static accept_pfn_t Hook_Func(accept) = (accept_pfn_t)dlsym(RTLD_NEXT, "accept");
static connect_pfn_t Hook_Func(connect) = (connect_pfn_t)dlsym(RTLD_NEXT, "connect");
static sendto_pfn_t Hook_Func(sendto) = (sendto_pfn_t)dlsym(RTLD_NEXT, "sendto");
static recvfrom_pfn_t Hook_Func(recvfrom) = (recvfrom_pfn_t)dlsym(RTLD_NEXT, "recvfrom");
static send_pfn_t Hook_Func(send) = (send_pfn_t)dlsym(RTLD_NEXT, "send");
static recv_pfn_t Hook_Func(recv) = (recv_pfn_t)dlsym(RTLD_NEXT, "recv");
static poll_pfn_t Hook_Func(poll) = (poll_pfn_t)dlsym(RTLD_NEXT, "poll");
static setsockopt_pfn_t Hook_Func(setsockopt) = (setsockopt_pfn_t)dlsym(RTLD_NEXT, "setsockopt");
static fcntl_pfn_t Hook_Func(fcntl) = (fcntl_pfn_t)dlsym(RTLD_NEXT, "fcntl");

#define Do_Hook(X)                                      \
    if (!Hook_Func(X)) {                                \
        Hook_Func(X) = (X##_pfn_t)dlsym(RTLD_NEXT, #X); \
    }
struct fileDesc {
    int _userFlags;
    struct sockaddr_in _addr;
    int _domain;

    struct timeval _Rtimeout; // for epoll
    struct timeval _Wtimeout; // for epoll
};

static inline bool isEnableHook()
{
    auto i = bScheduler::get();
    if (i->occupyRoutine->IsProgress)
        return false;
    else
        return i->occupyRoutine->IsEnableHook;
}

static struct fileDesc* hooksFdSet[HookContralFdNums] = {};
// fd 是系统级资源 自增 不用加锁
// 控制好 free 和 close 的时机
static inline fileDesc* allocFileDesc(int fd)
{
    fileDesc* i = (fileDesc*)calloc(1, sizeof(fileDesc));
    i->_addr;
    i->_domain = -1;
    i->_userFlags = 0;
    i->_Rtimeout.tv_sec = HookContralFdTimeout_S;
    i->_Rtimeout.tv_usec = HookContralFdTimeout_MS;
    i->_Wtimeout.tv_sec = HookContralFdTimeout_S;
    i->_Wtimeout.tv_usec = HookContralFdTimeout_MS;
    hooksFdSet[fd] = i;
    return i;
}
static inline fileDesc* getFileDesc(int fd)
{
    if (fd > -1 && fd < HookContralFdNums) {
        auto i = hooksFdSet[fd];
    }
    return nullptr;
}
static inline void freeFileDesc(int fd)
{
    if (fd > -1 && fd < HookContralFdNums && hooksFdSet[fd]) {
        free(hooksFdSet[fd]);
        hooksFdSet[fd] = nullptr;
    }
}
// struct pollfd
//{
// int fd;			/* File descriptor to poll.  */
// short int events;		/* Types of events poller cares about.  */
// short int revents;		/* Types of events that actually occurred.  */
//};
// 将 epoll 事件转换为 poll 事件
short epoll_to_poll_events(uint32_t epoll_events)
{
    short poll_events = 0;

    if (epoll_events & EPOLLIN)
        poll_events |= POLLIN;
    if (epoll_events & EPOLLOUT)
        poll_events |= POLLOUT;
    if (epoll_events & EPOLLPRI)
        poll_events |= POLLPRI;
    if (epoll_events & EPOLLERR)
        poll_events |= POLLERR;
    if (epoll_events & EPOLLHUP)
        poll_events |= POLLHUP;
    if (epoll_events & EPOLLRDHUP)
        poll_events |= POLLRDHUP;

    return poll_events;
}

// 将 poll 事件转换为 epoll 事件
uint32_t poll_to_epoll_events(short poll_events)
{
    uint32_t epoll_events = 0;

    if (poll_events & POLLIN)
        epoll_events |= EPOLLIN;
    if (poll_events & POLLOUT)
        epoll_events |= EPOLLOUT;
    if (poll_events & POLLPRI)
        epoll_events |= EPOLLPRI;
    if (poll_events & POLLERR)
        epoll_events |= EPOLLERR;
    if (poll_events & POLLHUP)
        epoll_events |= EPOLLHUP;
    if (poll_events & POLLRDHUP)
        epoll_events |= EPOLLRDHUP;

    return epoll_events;
}

void* pollPerpare(void* args, void* epoll_revent);
void* pollCallback(void* args);
void* pollTimeout(void* args);
struct poll_message {
    poll_message(struct pollfd* fds, nfds_t nfds)
    {
        this->_pollfds = fds;
        this->fdSize = nfds;
        // ProcessBegin delete
        task = TaskItem::New();
        task->self = bScheduler::get()->occupyRoutine;
        eachFdTask = (TaskItem**)calloc(this->fdSize, sizeof(TaskItem*));
        for (int i = 0; i < this->fdSize; i++) {
            eachFdTask[i] = TaskItem::New();
            eachFdTask[i]->bEpollEvent.data.ptr = eachFdTask[i];
            eachFdTask[i]->bEpollEvent.events = poll_to_epoll_events(_pollfds[i].events);
            eachFdTask[i]->epollFd = _pollfds[i].fd;
            eachFdTask[i]->callbackFunArgs = this;
            eachFdTask[i]->callbackFuncPtr = pollCallback;
            eachFdTask[i]->perpareFuncArgs = this;
            eachFdTask[i]->perpareFuncPtr = pollPerpare;
        }
        task->timeoutCallFunc = pollTimeout;
        task->timeoutCallFuncArgs = this;
    }
    ~poll_message()
    {
        free(eachFdTask);
    }
    size_t fdSize = 0;
    pollfd* _pollfds = nullptr;
    // ProcessBegin delete
    TaskItem* task;
    TaskItem** eachFdTask = nullptr;
    size_t _activeFdsNums = 0;

    // 递减到0 时加入就绪队列
    // 防止 一次epoll 触发多个文件描述符
    // 在多线程环境下 进行竞争状态 , 在寻址前析构 造成野指针
    size_t _activeFdsNums2 = 0;
    char IsJoinActiveList = 0, IsRemoveFromTimeoutList = 0;
};
void* pollTimeout(void* args)
{
    poll_message* pm = (poll_message*)args;
    auto Env = bRoutineEnv::get();
    for (int i = 0; i < pm->fdSize; i++) {
        epoll_ctl(Env->Epoll->epollFd, EPOLL_CTL_DEL, pm->eachFdTask[i]->epollFd, &pm->eachFdTask[i]->bEpollEvent);
    }
    return 0;
}
void* pollPerpare(void* args, void* epoll_revent)
{
    poll_message* pm = (poll_message*)args;
    epoll_event* ee = (epoll_event*)epoll_revent;
    auto targetFd = ((TaskItem*)ee->data.ptr)->epollFd;
    for (auto i = 0; i < pm->fdSize; i++) {
        if (pm->_pollfds[i].fd == targetFd) {
            pm->_pollfds[i].revents = epoll_to_poll_events(ee->events);
            break;
        }
    }
    pm->_activeFdsNums++;
    pm->_activeFdsNums2++;
    if (!pm->IsRemoveFromTimeoutList) {
        pm->IsRemoveFromTimeoutList = 1;
        RemoveSelfFromOwnerlink(pm->task);
    }
    return 0;
}
void* pollCallback(void* args)
{
    poll_message* pm = (poll_message*)args;
    pm->_activeFdsNums2--;
    if (!pm->IsJoinActiveList && pm->_activeFdsNums2 == 0) {
        pm->IsJoinActiveList = 1;
        bRoutineEnv::get()->TaskDistributor->AddTask(pm->task);
        pm->task = nullptr;
    }
    return 0;
}
int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
    Do_Hook(poll);
    if (!isEnableHook() || timeout == 0)
        return Hook_Func(poll)(fds, nfds, timeout);
    // 这里是 pm 存的是 fds 的指针
    // 回调时 操作 pm 中的 fds 的指针
    // 直接操作fds的数据 不用copy
    // 是私有栈区
    poll_message pm(fds, nfds);
    auto Env = bRoutineEnv::get();
    Env->Epoll->TimeoutScaner->getBucket((timeout > 0 ? timeout : Env->Epoll->TimeoutScaner->TaskItemBucketSize - 1) + bRoutineEnv::Timer::getNow_Ms())->pushBack(pm.task);
    for (int i = 0; i < pm.fdSize; i++) {
        epoll_ctl(Env->Epoll->epollFd, EPOLL_CTL_ADD, pm.eachFdTask[i]->epollFd, &pm.eachFdTask[i]->bEpollEvent);
    }
    bScheduler::get()->SwapContext();
    // 返回的时候 通过回调 操作过了 fds参数
    if (!pm.task->IsTasktimeout)
        for (int i = 0; i < pm.fdSize; i++) {
            epoll_ctl(Env->Epoll->epollFd, EPOLL_CTL_DEL, pm.eachFdTask[i]->epollFd, &pm.eachFdTask[i]->bEpollEvent);
        }
    auto i = pm._activeFdsNums;
    return i;
}
ssize_t read(int fd, void* buf, size_t nbyte)
{
    Do_Hook(read);
    if (!isEnableHook()) {
        return Hook_Func(read)(fd, buf, nbyte);
    }
    auto i = getFileDesc(fd);
    if (!i || i->_userFlags & O_NONBLOCK) {
        return Hook_Func(read)(fd, buf, nbyte);
    }
    int timeout = (i->_Rtimeout.tv_sec * 1000) + (i->_Rtimeout.tv_usec / 1000);
    struct pollfd pf = { 0 };
    pf.fd = fd;
    pf.events = (POLLIN | POLLERR | POLLHUP);
    int pollret = poll(&pf, 1, timeout);
    ssize_t readret = Hook_Func(read)(fd, (char*)buf, nbyte);
    if (readret < 0) {
        // LOG
    }
    return readret;
}
ssize_t write(int fd, const void* buf, size_t nbyte)
{
    Do_Hook(write);

    if (!isEnableHook()) {
        return Hook_Func(write)(fd, buf, nbyte);
    }
    auto* lp = getFileDesc(fd);

    if (!lp || (O_NONBLOCK & lp->_userFlags)) {
        return Hook_Func(write)(fd, buf, nbyte);
    }
    size_t wrotelen = 0;
    int timeout = (lp->_Wtimeout.tv_sec * 1000) + (lp->_Wtimeout.tv_usec / 1000);

    ssize_t writeret = Hook_Func(write)(fd, (const char*)buf + wrotelen, nbyte - wrotelen);

    if (writeret == 0) {
        return writeret;
    }

    if (writeret > 0) {
        wrotelen += writeret;
    }
    while (wrotelen < nbyte) {
        struct pollfd pf = { 0 };
        pf.fd = fd;
        pf.events = (POLLOUT | POLLERR | POLLHUP);
        poll(&pf, 1, timeout);

        writeret = Hook_Func(write)(fd, (const char*)buf + wrotelen, nbyte - wrotelen);

        if (writeret <= 0) {
            break;
        }
        wrotelen += writeret;
    }
    if (writeret <= 0 && wrotelen == 0) {
        return writeret;
    }
    return wrotelen;
}
int fcntl(int fildes, int cmd, ...)
{
    Do_Hook(fcntl);

    if (fildes < 0) {
        return __LINE__;
    }

    va_list arg_list;
    va_start(arg_list, cmd);

    int ret = -1;
    fileDesc* lp = getFileDesc(fildes);
    switch (cmd) {
    case F_DUPFD: {
        int param = va_arg(arg_list, int);
        ret = Hook_Func(fcntl)(fildes, cmd, param);
        break;
    }
    case F_GETFD: {
        ret = Hook_Func(fcntl)(fildes, cmd);
        break;
    }
    case F_SETFD: {
        int param = va_arg(arg_list, int);
        ret = Hook_Func(fcntl)(fildes, cmd, param);
        break;
    }
    case F_GETFL: {
        ret = Hook_Func(fcntl)(fildes, cmd);
        if (lp && !(lp->_userFlags & O_NONBLOCK)) {
            ret = ret & (~O_NONBLOCK);
        }
        break;
    }
    case F_SETFL: {
        int param = va_arg(arg_list, int);
        int flag = param;
        if (isEnableHook() && lp) {
            flag |= O_NONBLOCK;
        }
        ret = Hook_Func(fcntl)(fildes, cmd, flag);
        if (0 == ret && lp) {
            lp->_userFlags = param;
        }
        break;
    }
    case F_GETOWN: {
        ret = Hook_Func(fcntl)(fildes, cmd);
        break;
    }
    case F_SETOWN: {
        int param = va_arg(arg_list, int);
        ret = Hook_Func(fcntl)(fildes, cmd, param);
        break;
    }
    case F_GETLK: {
        struct flock* param = va_arg(arg_list, struct flock*);
        ret = Hook_Func(fcntl)(fildes, cmd, param);
        break;
    }
    case F_SETLK: {
        struct flock* param = va_arg(arg_list, struct flock*);
        ret = Hook_Func(fcntl)(fildes, cmd, param);
        break;
    }
    case F_SETLKW: {
        struct flock* param = va_arg(arg_list, struct flock*);
        ret = Hook_Func(fcntl)(fildes, cmd, param);
        break;
    }
    }

    va_end(arg_list);

    return ret;
}
int socket(int domain, int type, int protocol)
{
    Do_Hook(socket);

    if (!isEnableHook()) {
        return Hook_Func(socket)(domain, type, protocol);
    }
    int fd = Hook_Func(socket)(domain, type, protocol);
    if (fd < 0) {
        return fd;
    }
    auto* lp = allocFileDesc(fd);
    lp->_domain = domain;

    fcntl(fd, F_SETFL, Hook_Func(fcntl)(fd, F_GETFL, 0));

    return fd;
}
int accept(int fd, struct sockaddr* addr, socklen_t* len)
{
    Do_Hook(accept);
    if (!isEnableHook()) {
        return Hook_Func(accept)(fd, addr, len);
    }
    int cli = Hook_Func(accept)(fd, addr, len);
    if (cli < 0) {
        return cli;
    }
    allocFileDesc(cli);
    return cli;
}
int connect(int fd, const struct sockaddr* address, socklen_t address_len)
{
    Do_Hook(connect);

    if (!isEnableHook()) {
        return Hook_Func(connect)(fd, address, address_len);
    }

    // 1.sys call
    int ret = Hook_Func(connect)(fd, address, address_len);

    auto* lp = getFileDesc(fd);
    if (!lp)
        return ret;

    if (sizeof(lp->_addr) >= address_len) {
        memcpy(&(lp->_addr), address, (int)address_len);
    }
    if (O_NONBLOCK & lp->_userFlags) {
        return ret;
    }

    if (!(ret < 0 && errno == EINPROGRESS)) {
        return ret;
    }

    // 2.wait
    int pollret = 0;
    struct pollfd pf = { 0 };

    for (int i = 0; i < 3; i++) // 25s * 3 = 75s
    {
        memset(&pf, 0, sizeof(pf));
        pf.fd = fd;
        pf.events = (POLLOUT | POLLERR | POLLHUP);

        pollret = poll(&pf, 1, 25000);

        if (1 == pollret) {
            break;
        }
    }

    if (pf.revents & POLLOUT) // connect succ
    {
        // 3.check getsockopt ret
        int err = 0;
        socklen_t errlen = sizeof(err);
        ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
        if (ret < 0) {
            return ret;
        } else if (err != 0) {
            errno = err;
            return -1;
        }
        errno = 0;
        return 0;
    }

    errno = ETIMEDOUT;
    return ret;
}
int close(int fd)
{
    Do_Hook(close);

    if (!isEnableHook()) {
        return Hook_Func(close)(fd);
    }

    freeFileDesc(fd);
    int ret = Hook_Func(close)(fd);

    return ret;
}
ssize_t sendto(int socket, const void* message, size_t length, int flags, const struct sockaddr* dest_addr,
    socklen_t dest_len)
{
    /*
        1.no enable sys call ? sys
        2.( !lp || lp is non block ) ? sys
        3.try
        4.wait
        5.try
    */
    Do_Hook(sendto);
    if (!isEnableHook()) {
        return Hook_Func(sendto)(socket, message, length, flags, dest_addr, dest_len);
    }
    auto* lp = getFileDesc(socket);
    if (!lp || (O_NONBLOCK & lp->_userFlags)) {
        return Hook_Func(sendto)(socket, message, length, flags, dest_addr, dest_len);
    }
    ssize_t ret = Hook_Func(sendto)(socket, message, length, flags, dest_addr, dest_len);
    if (ret < 0 && EAGAIN == errno) {
        int timeout = (lp->_Wtimeout.tv_sec * 1000) + (lp->_Wtimeout.tv_usec / 1000);
        struct pollfd pf = { 0 };
        pf.fd = socket;
        pf.events = (POLLOUT | POLLERR | POLLHUP);
        poll(&pf, 1, timeout);
        ret = Hook_Func(sendto)(socket, message, length, flags, dest_addr, dest_len);
    }
    return ret;
}
ssize_t recvfrom(int socket, void* buffer, size_t length, int flags, struct sockaddr* address, socklen_t* address_len)
{
    Do_Hook(recvfrom);
    if (!isEnableHook()) {
        return Hook_Func(recvfrom)(socket, buffer, length, flags, address, address_len);
    }

    auto* lp = getFileDesc(socket);
    if (!lp || (O_NONBLOCK & lp->_userFlags)) {
        return Hook_Func(recvfrom)(socket, buffer, length, flags, address, address_len);
    }

    int timeout = (lp->_Rtimeout.tv_sec * 1000) + (lp->_Rtimeout.tv_usec / 1000);

    struct pollfd pf = { 0 };
    pf.fd = socket;
    pf.events = (POLLIN | POLLERR | POLLHUP);
    poll(&pf, 1, timeout);

    ssize_t ret = Hook_Func(recvfrom)(socket, buffer, length, flags, address, address_len);
    return ret;
}
ssize_t send(int socket, const void* buffer, size_t length, int flags)
{
    Do_Hook(send);

    if (!isEnableHook()) {
        return Hook_Func(send)(socket, buffer, length, flags);
    }
    auto* lp = getFileDesc(socket);

    if (!lp || (O_NONBLOCK & lp->_userFlags)) {
        return Hook_Func(send)(socket, buffer, length, flags);
    }
    size_t wrotelen = 0;
    int timeout = (lp->_Wtimeout.tv_sec * 1000) + (lp->_Wtimeout.tv_usec / 1000);

    ssize_t writeret = Hook_Func(send)(socket, buffer, length, flags);
    if (writeret == 0) {
        return writeret;
    }

    if (writeret > 0) {
        wrotelen += writeret;
    }
    while (wrotelen < length) {

        struct pollfd pf = { 0 };
        pf.fd = socket;
        pf.events = (POLLOUT | POLLERR | POLLHUP);
        poll(&pf, 1, timeout);

        writeret = Hook_Func(send)(socket, (const char*)buffer + wrotelen, length - wrotelen, flags);

        if (writeret <= 0) {
            break;
        }
        wrotelen += writeret;
    }
    if (writeret <= 0 && wrotelen == 0) {
        return writeret;
    }
    return wrotelen;
}
ssize_t recv(int socket, void* buffer, size_t length, int flags)
{
    Do_Hook(recv);

    if (!isEnableHook()) {
        return Hook_Func(recv)(socket, buffer, length, flags);
    }
    auto* lp = getFileDesc(socket);

    if (!lp || (O_NONBLOCK & lp->_userFlags)) {
        return Hook_Func(recv)(socket, buffer, length, flags);
    }
    int timeout = (lp->_Rtimeout.tv_sec * 1000) + (lp->_Rtimeout.tv_usec / 1000);

    struct pollfd pf = { 0 };
    pf.fd = socket;
    pf.events = (POLLIN | POLLERR | POLLHUP);

    int pollret = poll(&pf, 1, timeout);

    ssize_t readret = Hook_Func(recv)(socket, buffer, length, flags);
    if (readret < 0) {
        // co_log_err("CO_ERR: read fd %d ret %ld errno %d poll ret %d timeout %d", socket, readret, errno, pollret,timeout);
    }
    return readret;
}
int setsockopt(int fd, int level, int option_name, const void* option_value, socklen_t option_len)
{
    Do_Hook(setsockopt);

    if (!isEnableHook()) {
        return Hook_Func(setsockopt)(fd, level, option_name, option_value, option_len);
    }
    auto* lp = getFileDesc(fd);

    if (lp && SOL_SOCKET == level) {
        struct timeval* val = (struct timeval*)option_value;
        if (SO_RCVTIMEO == option_name) {
            memcpy(&lp->_Rtimeout, val, sizeof(*val));
        } else if (SO_SNDTIMEO == option_name) {
            memcpy(&lp->_Wtimeout, val, sizeof(*val));
        }
    }
    return Hook_Func(setsockopt)(fd, level, option_name, option_value, option_len);
}
// typedef struct tm* (*localtime_r_pfn_t)(const time_t* timep, struct tm* result);
// typedef void* (*pthread_getspecific_pfn_t)(pthread_key_t key);
// typedef int (*pthread_setspecific_pfn_t)(pthread_key_t key, const void* value);
// typedef int (*setenv_pfn_t)(const char* name, const char* value, int overwrite);
// typedef int (*unsetenv_pfn_t)(const char* name);
// typedef char* (*getenv_pfn_t)(const char* name);
// typedef hostent* (*gethostbyname_pfn_t)(const char* name);
// typedef res_state (*__res_state_pfn_t)();
// typedef int (*__poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
// static setenv_pfn_t Hook_Func(setenv) = (setenv_pfn_t)dlsym(RTLD_NEXT, "setenv");
// static unsetenv_pfn_t Hook_Func(unsetenv) = (unsetenv_pfn_t)dlsym(RTLD_NEXT, "unsetenv");
// static getenv_pfn_t Hook_Func(getenv) = (getenv_pfn_t)dlsym(RTLD_NEXT, "getenv");
// static __res_state_pfn_t Hook_Func(__res_state) = (__res_state_pfn_t)dlsym(RTLD_NEXT, "__res_state");
// static gethostbyname_pfn_t Hook_Func(gethostbyname) = (gethostbyname_pfn_t)dlsym(RTLD_NEXT, "gethostbyname");
// static __poll_pfn_t Hook_Func(__poll) = (__poll_pfn_t)dlsym(RTLD_NEXT, "__poll");
