#pragma once

#include "scheduler.h"
#include "mutex.h"
#include "timer.h"
#include <memory>
#include <vector>

namespace server{

class IOManager : public Scheduler, public TimerManager{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    enum Event
    {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4,
    };

    IOManager(size_t threads = 1, bool use_caller = false, const std::string &name ="");
    ~IOManager();

    //cancel和del的区别是删除不会触发事件，cancel会触发一次事件。
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);
    bool cancelAll(int fd);

    static IOManager *GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;

    bool stopping(uint64_t &timeout);

private:
    struct FdContext{
        typedef Mutex MutexType;

        struct EventContext{
            Scheduler *scheduler = nullptr;
            Fiber::ptr fiber;
            std::function<void()> cb;
        };

        EventContext &getEventContext(Event event);
        void resetEventContext(EventContext& event_ctx);
        void triggerEvent(Event event);

        int fd = 0;
        Event events = NONE;
        EventContext read;
        EventContext write;
        MutexType mutex;
    };

    int m_epfd = 0;
    int m_tickleFds[2];

    void contextResize(size_t len);

    std::vector<FdContext *> m_fdContexts;
    // 当前等待执行的事件数量
    std::atomic<size_t> m_pendingEventCount = {0};
    RWMutexType m_mutex;
};
}