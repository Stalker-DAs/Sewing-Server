#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <sys/epoll.h>
#include <fcntl.h>

namespace server{

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

IOManager::FdContext::EventContext &IOManager::FdContext::getEventContext(IOManager::Event event){
    switch(event){
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            ASSERT2(false, "getContext");
        }
}

void IOManager::FdContext::resetEventContext(IOManager::FdContext::EventContext& event_ctx){
    event_ctx.scheduler = nullptr;
    event_ctx.fiber.reset();
    event_ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event){

    ASSERT(event & events);

    //从events中去除该事件
    events = (Event)(events & ~event);
    EventContext &ctx = getEventContext(event);
    if(ctx.cb){
        ctx.scheduler->scheduler(&ctx.cb);
    }
    else{
        ctx.scheduler->scheduler(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
{
    m_epfd = epoll_create(5000);
    ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    ASSERT(!rt);

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    ASSERT(!rt);

    contextResize(32);

    start();
}

IOManager::~IOManager(){
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i){
        if (m_fdContexts[i]){
            delete (m_fdContexts[i]);
        }
    }
}

void IOManager::contextResize(size_t len){
    m_fdContexts.resize(len);
    for (size_t i = 0; i < len; ++i){
        if (!m_fdContexts[i]){
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb){
    FdContext *fd_ctx = nullptr;

    RWMutexType::ReadLock lock(m_mutex);
    if (fd >= m_fdContexts.size())
    {
        lock.unlock();
        RWMutexType::WriteLock lock(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }
    else{
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    //检查fd_ctx的events是否已经有事件了
    if(fd_ctx->events & event){
        LOG_ERROR(g_logger) << "addEvent assert fd=" << fd 
                                  << " event=" << event
                                  << " fd_ctx.event=" << fd_ctx->events;
        ASSERT(!(fd_ctx->events & event));
    }

    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = fd_ctx->events | event | EPOLLET;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt){
        LOG_ERROR(g_logger) << "epoller_ctl(" << m_epfd << ", " << op
                                  << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext &event_ctx = fd_ctx->getEventContext(event);
    //确保没有内容
    ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    event_ctx.scheduler = Scheduler::GetThis();
    if (cb){
        event_ctx.cb.swap(cb);
    }
    else{
        event_ctx.fiber = Fiber::GetThis();
        ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
    }

    return 0;
}

bool IOManager::delEvent(int fd, Event event){
    FdContext *fd_ctx = nullptr;

    RWMutexType::ReadLock lock(m_mutex);
    if (fd >= m_fdContexts.size())
    {
        return false;
    }
    fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //查看事件是否被注册
    if (!(fd_ctx->events & event)){
        return false;
    }

    //检查还有没有其他事件
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt){
        LOG_ERROR(g_logger) << "epoller_ctl(" << m_epfd << ", " << op
                                  << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;
    if (new_events == NONE){
        FdContext::EventContext &event_ctx = fd_ctx->getEventContext(event);
        fd_ctx->resetEventContext(event_ctx);
    }

    return true;
}

bool IOManager::cancelEvent(int fd, Event event){
    FdContext *fd_ctx = nullptr;

    RWMutexType::ReadLock lock(m_mutex);
    if (fd >= m_fdContexts.size())
    {
        return false;
    }
    fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //查看事件是否被注册
    if (!(fd_ctx->events & event)){
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt){
        LOG_ERROR(g_logger) << "epoller_ctl(" << m_epfd << ", " << op
                                  << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd){
    FdContext *fd_ctx = nullptr;

    RWMutexType::ReadLock lock(m_mutex);
    if (fd >= m_fdContexts.size())
    {
        return false;
    }
    fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    //查看事件是否被注册
    if (!(fd_ctx->events)){
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt){
        LOG_ERROR(g_logger) << "epoller_ctl(" << m_epfd << ", " << op
                                  << "," << fd << "," << epevent.events << "):"
                                  << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if (fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    ASSERT(fd_ctx->events == NONE);
    return true;
}

IOManager *IOManager::GetThis(){
    return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

void IOManager::tickle(){
    if (!hasIdleThreads()){
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    ASSERT(rt == 1);
}

bool IOManager::stopping(){
    uint64_t timeout = 0;
    return stopping(timeout);
}

bool IOManager::stopping(uint64_t &timeout){
    timeout = getNextTimer();
    //等待处理的事件为空，计时器中没有其他事件等待。
    return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
}

void IOManager::idle(){
    epoll_event *events = new epoll_event[64]();

    while (true){
        uint64_t next_timeout = 0;
        if (stopping(next_timeout)){
            LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
            break;
        }

        int rt = 0;
        do{
            static const int MAX_TIMEOUT = 3000;

            if (next_timeout!=~0ull){
                next_timeout = (int)next_timeout > MAX_TIMEOUT 
                                ? MAX_TIMEOUT : next_timeout;
            }else{
                next_timeout = MAX_TIMEOUT;
            }

            rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
            //LOG_INFO(g_logger) << "epoll_wait rt=" << rt;
            if (rt < 0 && errno == EINTR){

            }
            else{
                break;
            }
        }while(true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if (!cbs.empty()){
            //SYLAR_LOG_INFO(g_logger) << "on timer cbs.size=" << cbs.size();
            scheduler(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (size_t i = 0; i < rt; ++i){
            epoll_event &event = events[i];
            if (event.data.fd == m_tickleFds[0]){
                // 因为是ET通知，所以这里用while防止读取不干净
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext *fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if (event.events & (EPOLLERR | EPOLLHUP)){
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_event = NONE;
            if (event.events & EPOLLIN){
                real_event |= READ;
            }
            if (event.events & EPOLLOUT){
                real_event |= WRITE;
            }

            if ((fd_ctx->events & real_event) == NONE){
                continue;
            }
            
            int left_events = (fd_ctx->events & ~real_event);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;


            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2){
                LOG_ERROR(g_logger) << "epoller_ctl(" << m_epfd << ", " << op
                                    << "," << fd_ctx->fd << "," << event.events << "):"
                                    << rt << " (" << errno << ") (" << strerror(errno) << ")";
            }

            if (real_event & READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (real_event & WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();

        
    }
}
}