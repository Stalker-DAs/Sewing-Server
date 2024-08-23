#pragma once

#include <ucontext.h>
#include <memory.h>
#include <memory>
#include <functional>
#include <iostream>
#include <atomic>

namespace server{

class Scheduler;
class Fiber: public std::enable_shared_from_this<Fiber>{
    friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State
    {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };

    Fiber(std::function<void()> cb, uint32_t stacksize = 0, bool use_caller = false);
    ~Fiber();

    void call();
    void back();
    void swapIn();
    void swapOut();

    State getState(){
        return m_state;
    };

    void setState(State state){
        m_state = state;
    };

    void reset(std::function<void()> cb);
    uint64_t getId() { return m_id; };

    static Fiber::ptr GetRootFiber();
    static void SetThis(Fiber* fiber);
    static Fiber::ptr GetThis();
    static void YieldToReady();
    static void YieldToHold();
    static void YieldToReadyBack();
    static void YieldToHoldBack();
    static uint64_t GetFiberId();

    static void MainFunc();
    static void CallMainFunc();

private:
    Fiber();

    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void *m_stack = nullptr;

    std::function<void()> m_cb;
};
}