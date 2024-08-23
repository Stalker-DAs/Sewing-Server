#include "hook.h"
#include "log.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "config.h"

#include <dlfcn.h>
#include <iostream>

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

namespace server{

static server::ConfigVar<int>::ptr g_tcp_connect_timeout =
       server::Config::AddData("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enble = false;

bool is_hook_enable(){
    return t_hook_enble;
}

void set_hook_enable(bool flag){
    t_hook_enble = flag;
}

static uint64_t s_connect_timeout = -1;

struct _HookIniter{
    _HookIniter(){
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getVal();

        g_tcp_connect_timeout->addListen(66666, [](const int &old_value, const int &new_value)
                                           { LOG_INFO(g_logger) << "tcp connect timeout changed from "<< old_value << " to "<< new_value;
                                            s_connect_timeout = new_value; });
    }

    void hook_init(){
        sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
        usleep_f = (usleep_fun)dlsym(RTLD_NEXT, "usleep");
        nanosleep_f = (nanosleep_fun)dlsym(RTLD_NEXT, "nanosleep");
        socket_f = (socket_fun)dlsym(RTLD_NEXT, "socket");
        connect_f = (connect_fun)dlsym(RTLD_NEXT, "connect");
        accept_f = (accept_fun)dlsym(RTLD_NEXT, "accept");
        read_f = (read_fun)dlsym(RTLD_NEXT, "read");
        readv_f = (readv_fun)dlsym(RTLD_NEXT, "readv");
        recv_f = (recv_fun)dlsym(RTLD_NEXT, "recv");
        recvfrom_f = (recvfrom_fun)dlsym(RTLD_NEXT, "recvfrom");
        recvmsg_f = (recvmsg_fun)dlsym(RTLD_NEXT, "recvmsg");
        write_f = (write_fun)dlsym(RTLD_NEXT, "write");
        writev_f = (writev_fun)dlsym(RTLD_NEXT, "writev");
        send_f = (send_fun)dlsym(RTLD_NEXT, "send");
        sendto_f = (sendto_fun)dlsym(RTLD_NEXT, "sendto");
        sendmsg_f = (sendmsg_fun)dlsym(RTLD_NEXT, "sendmsg");
        close_f = (close_fun)dlsym(RTLD_NEXT, "close");
        fcntl_f = (fcntl_fun)dlsym(RTLD_NEXT, "fcntl");
        ioctl_f = (ioctl_fun)dlsym(RTLD_NEXT, "ioctl");
        getsockopt_f = (getsockopt_fun)dlsym(RTLD_NEXT, "getsockopt");
        setsockopt_f = (setsockopt_fun)dlsym(RTLD_NEXT, "setsockopt");

    }
};

static _HookIniter s_hook_initer;

}

struct timer_info{
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name, 
        uint32_t event, int timeout_so, Args&&... args)
{
    if (!server::is_hook_enable()){
        return fun(fd, std::forward<Args>(args)...);
    }

    //LOG_INFO(g_logger) << "do_io<" << hook_fun_name << ">";

    server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);

    if (!ctx)
    {
        return fun(fd, std::forward<Args>(args)...);
    }

    if (ctx->isClose()){
        errno = EBADF;
        return -1;
    }

    if (!ctx->isSocket() || ctx->getUserNonblock()){
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

//retry:
    while (1){
        ssize_t n = fun(fd, std::forward<Args>(args)...);
        while (n == -1 && errno == EINTR){
            n = fun(fd, std::forward<Args>(args)...);
        }
        if (n == -1 && errno == EAGAIN){
            server::IOManager* iom = server::IOManager::GetThis();
            server::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);

            if (to != (uint64_t)-1){
                timer = iom->addConditionTimer(to, [winfo, fd, iom, event]()
                { 
                    auto t = winfo.lock();
                    if (!t || t->cancelled){
                        return;
                    }
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (server::IOManager::Event)(event)); 
                }, winfo);
            }
            int rt = iom->addEvent(fd, (server::IOManager::Event)(event));
            //如果添加错误
            if (rt){
                LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                                        << fd << ", " << event << ")";
                if (timer)
                {
                    timer->getManager()->cancel(timer);
                }
                return -1;
            }
            else{
                // LOG_INFO(g_logger) << "do_io<" << hook_fun_name << ">";
                server::Fiber::YieldToHold();
                // LOG_INFO(g_logger) << "do_io<" << hook_fun_name << ">";
                if (timer){
                    timer->getManager()->cancel(timer);
                }
                if (tinfo->cancelled){
                    errno = tinfo->cancelled;
                    return -1;
                    //执行了计时器的回调函数，超时显示错误。
                }
                continue;
                // goto retry;
            }

        }
        return n;
    }
}

extern "C"{

sleep_fun sleep_f = nullptr;
usleep_fun usleep_f = nullptr;
nanosleep_fun nanosleep_f = nullptr;

socket_fun socket_f = nullptr;
connect_fun connect_f = nullptr;
accept_fun accept_f = nullptr;

read_fun read_f = nullptr;
readv_fun readv_f = nullptr;
recv_fun recv_f = nullptr;
recvfrom_fun recvfrom_f = nullptr;
recvmsg_fun recvmsg_f = nullptr;

write_fun write_f = nullptr;
writev_fun writev_f = nullptr;
send_fun send_f = nullptr;
sendto_fun sendto_f = nullptr;
sendmsg_fun sendmsg_f = nullptr;

close_fun close_f = nullptr;
fcntl_fun fcntl_f = nullptr;
ioctl_fun ioctl_f = nullptr;
getsockopt_fun getsockopt_f = nullptr;
setsockopt_fun setsockopt_f = nullptr;

unsigned int sleep(unsigned int seconds){
    if (!server::is_hook_enable()){
        sleep_f(seconds);
    }
    server::IOManager *iom = server::IOManager::GetThis();
    server::Fiber::ptr fiber = server::Fiber::GetThis();

    iom->addTimer(seconds * 1000, [iom, fiber]()
                   { iom->scheduler(fiber); });
    // iom->addTimer(seconds * 1000, [iom, fiber](int thread = -1) {
    //     iom->scheduler(fiber, thread);
    // });

    server::Fiber::YieldToHold();
    return 0;
}

//微秒
int usleep(useconds_t usec){
    if (!server::is_hook_enable()){
        sleep_f(usec);
    }
    
    server::IOManager *iom = server::IOManager::GetThis();
    server::Fiber::ptr fiber = server::Fiber::GetThis();

    iom->addTimer(usec / 1000, [iom, fiber]()
                   { iom->scheduler(fiber); });
    // iom->addTimer(seconds * 1000, [iom, fiber](int thread = -1) {
    //     iom->scheduler(fiber, thread);
    // });

    server::Fiber::YieldToHold();
    return 0;
}

//纳秒
int nanosleep(const struct timespec *req, struct timespec *rem){
    if (!server::is_hook_enable()){
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;

    server::Fiber::ptr fiber = server::Fiber::GetThis();
    server::IOManager *iom = server::IOManager::GetThis();

    iom->addTimer(timeout_ms / 1000, [iom, fiber]()
                   { iom->scheduler(fiber); });

    server::Fiber::YieldToHold();
    return 0;
}

int socket(int domain, int type, int protocol){
    int fd = socket_f(domain, type, protocol);
    if (!server::is_hook_enable() || fd == -1){
        return fd;
    }
    server::FdMgr::GetInstance()->add(fd);

    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms){
    if (!server::is_hook_enable()){
        return connect_f(fd, addr, addrlen);
    }
    server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->add(fd);
    if (!ctx || ctx->isClose()){
        errno = EBADF;
        return -1;
    }
    if(!ctx->isSocket()){
        return connect_f(fd, addr, addrlen);
    }
    if (ctx->getUserNonblock()){
        return connect_f(fd, addr, addrlen);
    }
    int n = connect_f(fd, addr, addrlen);
    if (n == 0){
        return 0;
    }
    else if(n != -1 || errno != EINPROGRESS){
        return n;
    }

    server::IOManager *iom = server::IOManager::GetThis();
    server::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if (timeout_ms != (uint64_t)-1){
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]()
                                       { auto t = winfo.lock();
                                       if (!t || t->cancelled){
                                           return;
                                       }
                                    t->cancelled = ETIMEDOUT;
                                    iom->cancelEvent(fd, server::IOManager::WRITE); }, winfo);
    
    }

    int rt = iom->addEvent(fd, server::IOManager::WRITE);
    if (rt == 0){
        server::Fiber::YieldToHold();
        if (timer){
            timer->getManager()->cancel(timer);
        }
        //连接超时
        if (tinfo->cancelled){
            errno = tinfo->cancelled;
            return -1;
        }
    }
    else{
        if (timer){
            timer->getManager()->cancel(timer);
        }
        LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)){
        return -1;
    }
    if (!error){
        return 0;
    }
    else{
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
    return connect_with_timeout(sockfd, addr, addrlen, server::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen){
    int fd = do_io(sockfd, accept_f, "accept", server::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if (fd>=0){
        server::FdMgr::GetInstance()->get(fd, true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count){
    return do_io(fd, read_f, "read", server::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, readv_f, "readv", server::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    return do_io(sockfd, recv_f, "recv", server::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen){
    return do_io(sockfd, recvfrom_f, "recvfrom", server::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    return do_io(sockfd, recvmsg_f, "recvmsg", server::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count){
    return do_io(fd, write_f, "write", server::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd, writev_f, "writev", server::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags){
    return do_io(sockfd, send_f, "send", server::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen){
    return do_io(sockfd, sendto_f, "sendto", server::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    return do_io(sockfd, sendmsg_f, "sendmsg", server::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd){
    if (!server::is_hook_enable()){
        return close_f(fd);
    }
    server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
    if (ctx){
        auto iom = server::IOManager::GetThis();
        if (iom){
            iom->cancelAll(fd);
        }
        server::FdMgr::GetInstance()->del(fd);
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd, ... /* arg */ ){
    va_list va;
    va_start(va, cmd);
    switch (cmd)
    {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket()){
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);
                if (ctx->getSysNonblock()){
                    arg |= O_NONBLOCK;
                }
                else{
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
                if (!ctx || ctx->isClose() || !ctx->isSocket()){
                    return arg;
                }
                if (ctx->getUserNonblock()){
                    return arg | O_NONBLOCK;
                }
                else{
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ:
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock *arg = va_arg(va, struct flock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int fd, unsigned long request, ...){
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    //FIONBIO（用于设置非阻塞I/O模式）
    if (FIONBIO == request){
        bool user_nonblock = !!*(int *)arg;
        server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(fd);
        if (!ctx || ctx->isClose()||!ctx->isSocket()){
            return ioctl_f(fd, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname,
                      void *optval, socklen_t *optlen){
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen){
    if (!server::is_hook_enable()){
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET){
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO){
            server::FdCtx::ptr ctx = server::FdMgr::GetInstance()->get(sockfd);
            if (ctx){
                const timeval *v = (const timeval *)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }

    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}