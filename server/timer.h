#pragma once

#include <memory>
#include <functional>
#include <set>
#include <vector>
#include "mutex.h"

namespace server{

class TimerManager;

class Timer : std::enable_shared_from_this<Timer>
{

friend class TimerManager;

public:
    typedef std::shared_ptr<Timer> ptr;

    TimerManager* getManager();

private:
    Timer(uint64_t interval, std::function<void()> cb, bool recurring, TimerManager* manager);

    bool m_recurring = false;
    // 执行周期
    uint64_t m_interval = 0;
    // 精确的事件结束事件
    uint64_t m_next = 0;
    std::function<void()> m_cb;
    TimerManager* m_manager;

    struct Comparetor{
        bool operator()(const Timer::ptr &lhs, const Timer::ptr &rhs);
    };
};

class TimerManager{
    friend class Timer;
public:
    typedef std::shared_ptr<TimerManager> ptr;
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t interval, std::function<void()> cb, bool recurring = false);

    //条件定时器，智能指针用来判断条件是否存在
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,
                                 bool recurring = false);
    
    uint64_t getNextTimer();

    void listExpiredCb(std::vector<std::function<void()>> &cbs);

    bool hasTimer();

    bool cancel(Timer::ptr timer);
    bool refresh(Timer::ptr timer);
    bool reset(Timer::ptr timer, uint64_t interval, bool from_now);

private:
    std::set<Timer::ptr, Timer::Comparetor> m_timers;
    RWMutexType m_mutex;
    bool m_tickled = false;
};
}