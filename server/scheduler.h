#pragma once

#include "log.h"
#include "fiber.h"
#include "macro.h"
#include "mutex.h"
#include "thread.h"
#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace server{
struct FiberAndCb{

    FiberAndCb(std::function<void()> cb, int thread = -1) : m_cb(cb), m_thread(thread){};
    FiberAndCb(std::function<void()>* cb, int thread = -1) : m_thread(thread){
        m_cb.swap(*cb);
    };

    FiberAndCb(Fiber::ptr fiber, int thread = -1) : m_fiber(fiber), m_thread(thread){};
    FiberAndCb(Fiber::ptr* fiber, int thread = -1) : m_thread(thread){
        m_fiber.swap(*fiber);
    };

    FiberAndCb() : m_thread(-1){};

    void reset(){
        m_cb = nullptr;
        m_fiber = nullptr;
        m_thread = -1;
    }

    std::function<void()> m_cb = nullptr;
    Fiber::ptr m_fiber = nullptr;
    int m_thread;
};

class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = false, const std::string& name = "");
    virtual ~Scheduler();

    template<class FiberOrCb>
    void scheduler(FiberOrCb fcb, int threadId = -1){
        bool need_tickle = false;
        FiberAndCb newfcb(fcb, threadId);
        {        
            MutexType::Lock lock(m_mutex);
            need_tickle = fiberQueue.empty();
            fiberQueue.push_back(newfcb);
        }

        if (need_tickle){
            tickle();
        }
    }

    template<class InputIterator>
    void scheduler(InputIterator begin, InputIterator end){
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = fiberQueue.empty();
            while (begin!=end){
                FiberAndCb newfcb(*begin, -1);
                fiberQueue.push_back(newfcb);
                ++begin;
            }
        }
        if (need_tickle){
            tickle();
        }
    }


    void start();
    void stop();

    void MainFunc();

    void setThis();
    std::string getName() { return m_name; };
    static Scheduler *GetThis(); // 获取当前的Scheduler
    static Fiber* GetMainFiber();

protected:
    virtual void tickle();
    virtual void idle();
    virtual bool stopping();

    bool hasIdleThreads() { return wait_threads > 0; };

private:
    std::vector<FiberAndCb> fiberQueue; //Fiber队列
    std::vector<Thread::ptr> threadQueue;     //线程Id队列
    std::vector<int> threadIdQueue;

    std::string m_name;                 //调度器名
    Fiber::ptr m_rootFiber;             //rootFiber指针
    size_t m_threadsNum;                //创建的线程数
    int m_rootThreadId;                 //线程Id
    bool m_use_caller;                  //是否将创建线程用于Mainfunc
    bool m_stopping;                    //控制是否停止

    MutexType m_mutex;                      //互斥锁，用于队列更新

    std::atomic<size_t> activate_threads = {0};  //活跃的线程数
    std::atomic<size_t> wait_threads = {0};      //停止的线程数
};
}

