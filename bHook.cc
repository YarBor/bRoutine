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

#include <map>
typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*connect_pfn_t)(int socket, const struct sockaddr* address, socklen_t address_len);
typedef int (*close_pfn_t)(int fd);
typedef ssize_t (*read_pfn_t)(int fildes, void* buf, size_t nbyte);
typedef ssize_t (*write_pfn_t)(int fildes, const void* buf, size_t nbyte);
typedef ssize_t (*sendto_pfn_t)(int socket, const void* message, size_t length, int flags,
    const struct sockaddr* dest_addr, socklen_t dest_len);
typedef ssize_t (*recvfrom_pfn_t)(int socket, void* buffer, size_t length, int flags, struct sockaddr* address,
    socklen_t* address_len);
typedef ssize_t (*send_pfn_t)(int socket, const void* buffer, size_t length, int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void* buffer, size_t length, int flags);
typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);
typedef int (*setsockopt_pfn_t)(int socket, int level, int option_name, const void* option_value, socklen_t option_len);
typedef int (*fcntl_pfn_t)(int fildes, int cmd, ...);
typedef struct tm* (*localtime_r_pfn_t)(const time_t* timep, struct tm* result);
typedef void* (*pthread_getspecific_pfn_t)(pthread_key_t key);
typedef int (*pthread_setspecific_pfn_t)(pthread_key_t key, const void* value);
typedef int (*setenv_pfn_t)(const char* name, const char* value, int overwrite);
typedef int (*unsetenv_pfn_t)(const char* name);
typedef char* (*getenv_pfn_t)(const char* name);
typedef hostent* (*gethostbyname_pfn_t)(const char* name);
typedef res_state (*__res_state_pfn_t)();
typedef int (*__poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);

#define Hook_Func(X) Hook_##X##_func

static read_pfn_t Hook_Func(read) = (read_pfn_t)dlsym(RTLD_NEXT, "read");
static write_pfn_t Hook_Func(write) = (write_pfn_t)dlsym(RTLD_NEXT, "write");
static close_pfn_t Hook_Func(close) = (close_pfn_t)dlsym(RTLD_NEXT, "close");
static socket_pfn_t Hook_Func(socket) = (socket_pfn_t)dlsym(RTLD_NEXT, "socket");
static connect_pfn_t Hook_Func(connect) = (connect_pfn_t)dlsym(RTLD_NEXT, "connect");
static sendto_pfn_t Hook_Func(sendto) = (sendto_pfn_t)dlsym(RTLD_NEXT, "sendto");
static recvfrom_pfn_t Hook_Func(recvfrom) = (recvfrom_pfn_t)dlsym(RTLD_NEXT, "recvfrom");
static send_pfn_t Hook_Func(send) = (send_pfn_t)dlsym(RTLD_NEXT, "send");
static recv_pfn_t Hook_Func(recv) = (recv_pfn_t)dlsym(RTLD_NEXT, "recv");
static poll_pfn_t Hook_Func(poll) = (poll_pfn_t)dlsym(RTLD_NEXT, "poll");
static setsockopt_pfn_t Hook_Func(setsockopt) = (setsockopt_pfn_t)dlsym(RTLD_NEXT, "setsockopt");
static fcntl_pfn_t Hook_Func(fcntl) = (fcntl_pfn_t)dlsym(RTLD_NEXT, "fcntl");
static setenv_pfn_t Hook_Func(setenv) = (setenv_pfn_t)dlsym(RTLD_NEXT, "setenv");
static unsetenv_pfn_t Hook_Func(unsetenv) = (unsetenv_pfn_t)dlsym(RTLD_NEXT, "unsetenv");
static getenv_pfn_t Hook_Func(getenv) = (getenv_pfn_t)dlsym(RTLD_NEXT, "getenv");
static __res_state_pfn_t Hook_Func(__res_state) = (__res_state_pfn_t)dlsym(RTLD_NEXT, "__res_state");
static gethostbyname_pfn_t Hook_Func(gethostbyname) = (gethostbyname_pfn_t)dlsym(RTLD_NEXT, "gethostbyname");
static __poll_pfn_t Hook_Func(__poll) = (__poll_pfn_t)dlsym(RTLD_NEXT, "__poll");

#define Do_Hook(X)                                      \
    if (!Hook_Func(X)) {                                \
        Hook_Func(X) = (X##_pfn_t)dlsym(RTLD_NEXT, #X); \
    }
struct fileDesc {
    int _fd;
    struct sockaddr_in* _addr;
    int _domain;

    struct timeval _Rtimeout; // for epoll
    struct timeval _Wtimeout; // for epoll
};
#define HookContralFdNums 102400
#define HookContralFdTimeout_S 1
#define HookContralFdTimeout_MS 0

static struct fileDesc* hooksFdSet[HookContralFdNums] = {};
// fd 是系统级资源 自增 不用加锁
// 控制好 free 和 close 的时机
static inline fileDesc* allocFileDesc(int fd)
{
    if (fd > -1 && fd < HookContralFdNums) {
        fileDesc* i = (fileDesc*)calloc(1, sizeof(fileDesc));
        i->_addr = nullptr;
        i->_domain = -1;
        i->_fd = fd;
        i->_Rtimeout.tv_sec = HookContralFdTimeout_S;
        i->_Rtimeout.tv_usec = HookContralFdTimeout_MS;
        i->_Wtimeout.tv_sec = HookContralFdTimeout_S;
        i->_Wtimeout.tv_usec = HookContralFdTimeout_MS;
    }
    return nullptr;
}
static inline fileDesc* getFileDesc(int fd)
{
    if (fd > -1 && fd < HookContralFdNums) {
        return hooksFdSet[fd];
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

ssize_t read(int fd, void* buf, size_t nbyte)
{
    Do_Hook(read);
    return 0;
}