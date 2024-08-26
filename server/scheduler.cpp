#include "scheduler.h"
#include "hook.h"

namespace server{

static server::Logger::ptr g_logger = LOG_GET_LOGGER("system");

// scheduler的指针
static thread_local Scheduler *t_scheduler = nullptr;
// 主fiber，也就是执行调度的fiber
static thread_local Fiber *t_fiber = nullptr;
// 线程连续执行任务池中的任务最大次数
static uint8_t frequency = 10;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name) : m_name(name), m_use_caller(use_caller)
{
    ASSERT(threads > 0);

    if (use_caller)
    {
        Fiber::GetThis();
        ASSERT(GetThis() == nullptr);

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::MainFunc, this), 0, true));
        --threads;

        t_fiber = m_rootFiber.get();
        m_rootThreadId = GetThreadId();
        threadIdQueue.push_back(m_rootThreadId);
    }
    else{
        m_rootThreadId = -1;
    }
    m_threadsNum = threads;
}

Scheduler::~Scheduler(){
    if (GetThis() == this){
        t_scheduler = nullptr;
    }
}

void Scheduler::start(){
    LOG_INFO(g_logger) << "Start() start!";
    {
        MutexType::Lock lock(m_mutex);

        ASSERT(threadQueue.empty());
        threadQueue.resize(m_threadsNum);
    }

    m_stopping = false;

    for (int i = 0; i < m_threadsNum; ++i)
    {
        MutexType::Lock lock(m_mutex);
        threadQueue[i].reset(new Thread("thread" + std::to_string(i), std::bind(&Scheduler::MainFunc, this)));
        threadIdQueue.push_back(threadQueue[i]->getId());
    }
}

void Scheduler::stop(){
    LOG_INFO(g_logger) << "Stop() start!";
    m_stopping = true;
    if (m_rootFiber && m_threadsNum == 0 && (m_rootFiber->getState() == Fiber::INIT || m_rootFiber->getState() == Fiber::TERM)){
        if (stopping()){
            return;
        }
    }
    
    // 唤醒所有线程
    for (size_t i = 0; i < m_threadsNum; ++i){
        tickle();
    }
    if (m_rootFiber){
        tickle();
    }

    if (m_rootThreadId != -1){
        if (!stopping()){
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(threadQueue);
    }

    for (auto& i : thrs){
        i->join();
    }

    return;
}

Fiber* Scheduler::GetMainFiber(){
    return t_fiber;
}


void Scheduler::MainFunc(){
    LOG_INFO(g_logger) << "MainFunc start!";
    set_hook_enable(true);
    setThis();
    //也就是没有主Fiber
    if (GetThreadId() != m_rootThreadId)
    {
        t_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

    Fiber::ptr tmp;
    FiberAndCb ret;

    uint8_t work_times = 0;

    while (true){
        ret.reset();
        bool tickle_threads = false;
        bool is_active = true;
        {
            MutexType::Lock lock(m_mutex);
            for (auto it = fiberQueue.begin(); it != fiberQueue.end(); ++it)
            {
                if (work_times == frequency){
                    break;
                }
                if (it->m_thread != -1 && it->m_thread!=GetThreadId()){
                    tickle_threads = true;
                    continue;
                }

                ASSERT(it->m_cb || it->m_fiber);

                if (it->m_fiber && it->m_fiber->getState() == Fiber::EXEC){
                    continue;
                }
                
                ret = *it;
                fiberQueue.erase(it);
                ++activate_threads;
                is_active = true;
                break;
            }
        }

        if (tickle_threads){
            tickle();
        }

        if (ret.m_fiber && (ret.m_fiber->getState() != Fiber::TERM && ret.m_fiber->getState() != Fiber::EXCEPT) && work_times != frequency){
            ++work_times;
            ret.m_fiber->swapIn();
            // ret.m_fiber->swapIn();
            --activate_threads;

            if (ret.m_fiber->getState() == Fiber::READY){
                scheduler(ret.m_fiber);
            }
            else if (ret.m_fiber->getState() != Fiber::EXCEPT && ret.m_fiber->getState() != Fiber::TERM){
                ret.m_fiber->setState(Fiber::HOLD);
            }
            ret.reset();
        }
        else if (ret.m_cb && work_times != frequency){
            ++work_times;
            //Fix
            if (tmp){
                tmp->reset(ret.m_cb);
            }
            else{
                tmp.reset(new Fiber(ret.m_cb));
            }
            ret.reset();

            //tmp.reset(new Fiber(ret.m_cb));
            tmp->swapIn();
            // tmp->swapIn();
            --activate_threads;

            if (tmp->getState() == Fiber::READY){
                scheduler(tmp);
                tmp.reset();
            }
            else if (tmp->getState() == Fiber::EXCEPT || tmp->getState() == Fiber::TERM){
                tmp->reset(nullptr);
            }
            else{
                tmp->m_state = Fiber::HOLD;
                tmp.reset();
            }
        }
        else{
            work_times = 0;

            if (is_active){
                --activate_threads;
            }

            if (idle_fiber->getState() == Fiber::TERM){
                LOG_INFO(g_logger) << "idle fiber term" << std::endl;
                break;
            }

            ++wait_threads;
            idle_fiber->swapIn();
            // idle_fiber->swapIn();
            --wait_threads;

            if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT){
                idle_fiber->setState(Fiber::HOLD);
            }
        }
    }
    
}

void Scheduler::setThis(){
    t_scheduler = this;
}

Scheduler *Scheduler::GetThis(){
    return t_scheduler;
}

//通知各线程退出idle
void Scheduler::tickle(){
    LOG_INFO(g_logger) << "tickle";
}

void Scheduler::idle(){
    LOG_INFO(g_logger) << "idle";
    while (!stopping()){
        Fiber::YieldToHold();
    }
}

bool Scheduler::stopping(){
    return m_stopping && fiberQueue.empty() && activate_threads == 0;
}
}
