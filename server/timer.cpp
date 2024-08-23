#include "timer.h"
#include "util.h"

namespace server{

bool Timer::Comparetor::operator()(const Timer::ptr &lhs, const Timer::ptr &rhs){
    if (!lhs && !rhs){
        return false;
    }
    if (!lhs){
        return true;
    }
    if (!rhs){
        return false;
    }
    if (lhs->m_next > rhs->m_next){
        return false;
    }
    if (lhs->m_next < rhs->m_next){
        return true;
    }

    return lhs.get() < rhs.get();
}

TimerManager *Timer::getManager(){
    return m_manager;
}

Timer::Timer(uint64_t interval, std::function<void()> cb, bool recurring, TimerManager* manager)
            : m_interval(interval), m_cb(cb), m_recurring(recurring), m_manager(manager)
{
    m_next = server::GetCurrentMS() + m_interval;
}

TimerManager::TimerManager(){

}

TimerManager::~TimerManager(){

}

Timer::ptr TimerManager::addTimer(uint64_t interval, std::function<void()> cb, bool recurring){
    Timer::ptr timer(new Timer(interval, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    
    //.first返回一个迭代器
    auto it = m_timers.insert(timer).first;
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if (at_front){
        m_tickled = true;
    }

    lock.unlock();

    if (at_front){
        //onTimerInsertedAtFront();
    }

    return timer;
}

bool TimerManager::cancel(Timer::ptr timer){
    RWMutexType::WriteLock lock(m_mutex);
    if (timer->m_cb){
        timer->m_cb = nullptr;
        auto it = m_timers.find(timer);
        if (it != m_timers.end()){
            m_timers.erase(it);
            return true;
        }
    }

    return false;
}

bool TimerManager::refresh(Timer::ptr timer){
    RWMutexType::WriteLock lock(m_mutex);
    if (!timer->m_cb){
        return false;
    }
    auto it = m_timers.find(timer);
    if (it == m_timers.end()){
        return false;
    }
    m_timers.erase(it);
    timer->m_next = server::GetCurrentMS() + timer->m_interval;
    m_timers.insert(timer);

    return true;
}

bool TimerManager::reset(Timer::ptr timer, uint64_t interval, bool from_now){
    if (timer->m_interval == interval & !from_now){
        return true;
    }
    RWMutexType::WriteLock lock(m_mutex);
    if (!timer->m_cb){
        return false;
    }
    auto it = m_timers.find(timer);
    if (it == m_timers.end()){
        return false;
    }
    m_timers.erase(it);
    uint64_t start = 0;
    if (from_now)
    {
        start = server::GetCurrentMS();
    }
    else{
        start = timer->m_next - timer->m_interval;
    }
    timer->m_interval = interval;
    timer->m_next = start + interval;
    m_timers.insert(timer);

    return true;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb){
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp){
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,
                                 bool recurring){
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
}
//到最近一个定时器执行的时间间隔(毫秒)
uint64_t TimerManager::getNextTimer(){
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if (m_timers.empty()){
        return ~0ull;
    }
    const Timer::ptr &next = *m_timers.begin();
    uint64_t now_ms = server::GetCurrentMS();
    if (now_ms >= next->m_next){
        return 0;
    }
    else{
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>> &cbs){
    uint64_t now_ms = server::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()){
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);

    Timer::ptr now_timer(new Timer(0, nullptr, false, this));
    //找到lowerbound
    auto it = m_timers.lower_bound(now_timer);
    //因为lowerbound没有考虑所有now_ms的，要把这些now_ms的都考虑到
    while(it!= m_timers.end() && (*it)->m_next == now_ms){
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for (auto& timer: expired){
        cbs.push_back(timer->m_cb);

        if (timer->m_recurring){
            timer->m_next = now_ms + timer->m_interval;
            m_timers.insert(timer);
        }
        else{
            timer->m_cb = nullptr;
        }
    }
}

bool TimerManager::hasTimer(){
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}

}